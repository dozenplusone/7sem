#include "async.h"
#include "shared_ptr.h"

#include <ctime>
#include <iostream>
#include <optional>
#include <random>
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
    std::vector<std::optional<size_t>> results(players.size());
    while (true) {
        for (size_t i = 0; i < players.size(); ++i) {
            results[i] = co_await players[i]->act(*this);
        }
        for (size_t i = 0; i < players.size(); ++i) {
            results[i] = co_await players[i]->vote(*this);
        }
        co_return;
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
        last_cured = result;
    }
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
