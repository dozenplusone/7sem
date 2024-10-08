#include "async.h"
#include "shared_ptr.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <optional>
#include <random>
#include <ranges>
#include <unordered_map>
#include <vector>

class Player;

class Game {
    std::vector<hw1::shared_ptr<Player>> players;
public:
    Game(size_t, bool = false);
    const Player *operator[](size_t i) const { return players.at(i).get(); }
    inline size_t random_choice(void) const;
    template<class Role>
    bool check_role(size_t i) const {
        return !!dynamic_cast<Role*>(players.at(i).get());
    }
    hw1::async<void> start(void);
    bool finished(void) const;
};

class Player {
    friend class Game;

protected:
    bool human;
    bool alive;

public:
    Player(bool is_human = false)
        : human(is_human)
        , alive(true)
    {}
    inline bool is_alive(void) const noexcept { return alive; }
    virtual hw1::async<std::optional<size_t>> vote(const Game&) = 0;
    virtual hw1::async<std::optional<size_t>> act(const Game&) = 0;
};

class Civilian: public Player {
public:
    Civilian(bool is_human = false)
        : Player(is_human)
    {}
    hw1::async<std::optional<size_t>> vote(const Game&) override;
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

class Mafioso: public Player {
public:
    Mafioso(bool is_human = false)
        : Player(is_human)
    {}
    hw1::async<std::optional<size_t>> vote(const Game&) override;
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

class Sheriff: public Civilian {
protected:
    std::optional<size_t> last_checked;
    bool has_target;

public:
    Sheriff(bool is_human = false)
        : Civilian(is_human)
        , last_checked{}
        , has_target(false)
    {}
    hw1::async<std::optional<size_t>> vote(const Game&) override;
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

class Doctor: public Civilian {
protected:
    std::optional<size_t> last_cured;

public:
    Doctor(bool is_human = false)
        : Civilian(is_human)
        , last_cured{}
    {}
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

class Maniac: public Player {
public:
    Maniac(bool is_human = false)
        : Player(is_human)
    {}
    hw1::async<std::optional<size_t>> vote(const Game&) override;
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

Game::Game(size_t n_players, bool human)
    : players(n_players, nullptr)
{
    size_t cur;

    std::clog << '\n';

    for (int i = 0; i < (n_players >> 2); ++i) {
        do {
            cur = random_choice();
        } while (players[cur]);
        players[cur] = hw1::shared_ptr<Player>(new Mafioso);
        std::clog << "Player #" << cur << " is Mafioso\n";
    }

    do {
        cur = random_choice();
    } while (players[cur]);
    players[cur] = hw1::shared_ptr<Player>(new Sheriff);
    std::clog << "Player #" << cur << " is Sheriff\n";

    do {
        cur = random_choice();
    } while (players[cur]);
    players[cur] = hw1::shared_ptr<Player>(new Doctor);
    std::clog << "Player #" << cur << " is Doctor\n";

    do {
        cur = random_choice();
    } while (players[cur]);
    players[cur] = hw1::shared_ptr<Player>(new Maniac);
    std::clog << "Player #" << cur << " is Maniac\n";

    for (cur = 0; cur < n_players; ++cur) {
        if (!players[cur]) {
            players[cur] = hw1::shared_ptr<Player>(new Civilian);
            std::clog << "Player #" << cur << " is Civilian\n";
        }
    }

    if (human) {
        cur = random_choice();
        players[cur]->human = true;
        std::clog << "Player #" << cur << " is human!\n";
    }
}

size_t Game::random_choice(void) const {
    static std::mt19937_64 rng(time(nullptr));
    static std::uniform_int_distribution<size_t> dist(0, players.size() - 1);
    return dist(rng);
}

hw1::async<void> Game::start(void) {
    std::vector<std::pair<size_t, size_t>> results;
    std::unordered_map<size_t, size_t> counter;
    size_t day = 0;
    while (true) {
        std::clog << ">>> Day #" << ++day << " <<<\n";
        results.clear();
        counter.clear();
        for (size_t i = 0; i < players.size(); ++i) {
            std::optional<size_t> vote = co_await players[i]->vote(*this);
            if (vote.has_value()) {
                ++counter[vote.value()];
            }
        }

        auto decision = std::max_element(
            counter.begin(),
            counter.end(),
            [](auto &lhs, auto &rhs) { return lhs.second > rhs.second; }
        );

        if (decision != counter.end()) {
            players[decision->first]->alive = false;
            std::clog << "Player #" << decision->first
                      << " got kicked out.\n";
        }

        if (finished()) {
            std::clog << "Game over.\n";
            co_return;
        }

        std::clog << ">>> Night #" << day << " <<<\n";
        results.clear();
        counter.clear();
        for (size_t i = 0; i < players.size(); ++i) {
            std::optional<size_t> action = co_await players[i]->act(*this);
            if (action.has_value()) {
                results.emplace_back(i, action.value());
            }
        }

        auto mafia_votes = results | std::ranges::views::filter(
            [this](const auto &obj) { return check_role<Mafioso>(obj.first); }
        );
        if (!mafia_votes.empty()) {
            auto mafia_counter = std::transform(
                mafia_votes.begin(),
                mafia_votes.end(),
                std::inserter(counter, counter.begin()),
                [&counter](const auto &obj) -> std::pair<size_t, size_t> {
                    return {obj.second, ++counter[obj.second]};
                }
            );
            auto mafia_decision = std::max_element(
                counter.begin(),
                counter.end(),
                [](auto &lhs, auto &rhs) { return lhs.second > rhs.second; }
            );
            players[mafia_decision->first]->alive = false;
            std::clog << "Player #" << mafia_decision->first
                    << " got killed by the mafia.\n";
        }

        auto maniac_decision = results | std::ranges::views::filter(
            [this](const auto &obj) { return check_role<Maniac>(obj.first); }
        );
        if (!maniac_decision.empty()) {
            players[maniac_decision.front().second]->alive = false;
            std::clog << "Player #" << maniac_decision.front().second
                      << " got killed by Maniac.\n";
        }

        auto sheriff_decision = results | std::ranges::views::filter(
            [this](const auto &obj) { return check_role<Sheriff>(obj.first); }
        );
        if (!sheriff_decision.empty()) {
            players[sheriff_decision.front().second]->alive = false;
            std::clog << "Player #" << sheriff_decision.front().second
                      << " got killed by Sheriff.\n";
        }

        auto doctor_decision = results | std::ranges::views::filter(
            [this](const auto &obj) { return check_role<Doctor>(obj.first); }
        );
        if (!doctor_decision.empty()) {
            players[doctor_decision.front().second]->alive = true;
            std::clog << "Player #" << doctor_decision.front().second
                      << " got healed by Doctor.\n";
        }

        if (finished()) {
            std::clog << "Game over.\n";
            co_return;
        }
    }
}

bool Game::finished(void) const {
    bool maniac_alive;
    size_t mafioso_count = 0;
    size_t civilian_count = 0;
    for (size_t i = 0; i < players.size(); ++i) {
        if (check_role<Maniac>(i)) {
            maniac_alive = players[i]->is_alive();
        } else if (check_role<Mafioso>(i)) {
            mafioso_count += players[i]->is_alive();
        } else {
            civilian_count += players[i]->is_alive();
        }
    }
    if (mafioso_count + civilian_count + maniac_alive == 0) {
        std::clog << "Drawn!\n";
        return true;
    } else if (mafioso_count == 0) {
        if (!maniac_alive) {
            std::clog << "Civilians win!\n";
            return true;
        } else if (civilian_count <= 1) {
            std::clog << "Maniac wins!\n";
            return true;
        } else {
            return false;
        }
    } else if (!maniac_alive && mafioso_count >= civilian_count) {
        std::clog << "Mafia wins!\n";
        return true;
    } else {
        return false;
    }
}

hw1::async<std::optional<size_t>> Civilian::vote(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    if (human) {
        std::cout << "vote: ";
        std::cin >> result;
    } else {
        do {
            result = game.random_choice();
        } while (game[result] == this || !game[result]->is_alive());
    }
    co_return result;
}

hw1::async<std::optional<size_t>> Mafioso::vote(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    if (human) {
        std::cout << "vote: ";
        std::cin >> result;
    } else {
        do {
            result = game.random_choice();
        } while (
            game.check_role<Mafioso>(result)
            || !game[result]->is_alive()
        );
    }
    co_return result;
}

hw1::async<std::optional<size_t>> Sheriff::vote(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    if (human) {
        std::cout << "vote: ";
        std::cin >> result;
    } else {
        if (!has_target) {
            do {
                result = game.random_choice();
            } while (game[result] == this || !game[result]->is_alive());
        } else {
            result = last_checked.value();
        }
    }
    co_return result;
}

hw1::async<std::optional<size_t>> Maniac::vote(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    if (human) {
        std::cout << "vote: ";
        std::cin >> result;
    } else {
        do {
            result = game.random_choice();
        } while (game[result] == this || !game[result]->is_alive());
    }
    co_return result;
}

hw1::async<std::optional<size_t>> Civilian::act(const Game&) {
    co_return {};
}

hw1::async<std::optional<size_t>> Mafioso::act(const Game &game) {
    co_return co_await vote(game);
}

hw1::async<std::optional<size_t>> Sheriff::act(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    if (human) {
        std::optional<bool> shooting{};
        do {
            std::cout << "[c]heck or [s]hoot? ";
            switch (std::getchar()) {
            case 'c': case 'C':
                shooting = false;
                break;
            case 's': case 'S':
                shooting = true;
                break;
            }
        } while (!shooting.has_value());
        std::cout << "vote: ";
        std::cin >> result;
        if (!shooting) {
            co_return {};
        } else {
            co_return result;
        }
    } else {
        if (has_target && !game[result]->is_alive()) {
            last_checked = std::nullopt;
            has_target = false;
        }
        if (!has_target) {
            do {
                result = game.random_choice();
            } while (game[result] == this || !game[result]->is_alive());
            if (game.check_role<Mafioso>(result)) {
                last_checked = result;
                has_target = true;
            }
            co_return {};
        } else {
            co_return last_checked;
        }
    }
}

hw1::async<std::optional<size_t>> Doctor::act(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    if (human) {
        std::cout << "vote: ";
        std::cin >> result;
        while (last_cured.has_value() && last_cured == result) {
            std::cout << "you can't\nvote: ";
            std::cin >> result;
        }
    } else {
        do {
            result = game.random_choice();
        } while (
            last_cured.has_value() && game[result] == game[last_cured.value()]
            || !game[result]->is_alive()
        );
    }
    last_cured = result;
    co_return result;
}

hw1::async<std::optional<size_t>> Maniac::act(const Game &game) {
    co_return co_await vote(game);
}

int main(int argc, const char *argv[]) {
    size_t n_players = 0;
    if (argc > 1) {
        n_players = strtoul(argv[1], nullptr, 10);
    }
    while (n_players < 5) {
        std::cout << "how many players? ";
        std::cin >> n_players;
    }
    bool human;
    if (argc > 2 && argv[2] == std::string("--human")) {
        human = true;
    } else {
        std::cout << "do u wanna play [Y] or just watch [enter]? ";
        human = std::getchar() == 'Y';
    }
    Game game(n_players, human);
    game.start();
    return 0;
}
