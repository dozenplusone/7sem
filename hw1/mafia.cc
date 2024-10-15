#include "async.h"
#include "logging.h"
#include "shared_ptr.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <optional>
#include <random>
#include <ranges>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Player;

class Game {
    std::vector<hw1::shared_ptr<Player>> players;
    static std::unordered_map<std::type_index, std::string_view> rolenames;
    hw1::Logger logger;
    size_t day;

public:
    Game(size_t, bool = false);
    const Player *operator[](size_t i) const { return players.at(i).get(); }
    constexpr size_t n_players(void) const { return players.size(); }

    inline size_t random_choice(void) const;
    template<class Role>
    bool check_role(size_t i) const {
        return !!dynamic_cast<Role*>(players.at(i).get());
    }

    hw1::async<void> start(void);
    bool finished(void) const;

    void log(const std::string&, size_t, size_t, bool = true) const;
};

class Player {
    friend class Game;

protected:
    size_t this_id;
    bool human;
    bool alive;

    size_t human_input(const Game&, const std::string&);
    virtual size_t computer_vote(const Game&) = 0;

public:
    Player(bool is_human = false)
        : human(is_human)
        , alive(true)
    {}
    virtual ~Player() {}
    bool is_alive(void) const noexcept { return alive; }
    virtual hw1::async<std::optional<size_t>> vote(const Game&);
    virtual hw1::async<std::optional<size_t>> act(const Game&) = 0;
};

class Civilian: public Player {
public:
    Civilian(bool is_human = false)
        : Player(is_human)
    {}
    size_t computer_vote(const Game &game) override {
        return Player::computer_vote(game);
    }
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

class Mafioso: public Player {
public:
    Mafioso(bool is_human = false)
        : Player(is_human)
    {}
    size_t computer_vote(const Game&) override;
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

class Sheriff: public Civilian {
protected:
    std::unordered_set<size_t> civilians_checked;
    std::optional<size_t> last_checked;
    bool has_target;

public:
    Sheriff(bool is_human = false)
        : Civilian(is_human)
        , last_checked{}
        , has_target(false)
    {}
    size_t computer_vote(const Game&) override;
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
    size_t computer_vote(const Game &game) override {
        return Player::computer_vote(game);
    }
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

std::unordered_map<std::type_index, std::string_view> Game::rolenames{
    {typeid(Civilian), "Civilian"},
    {typeid(Mafioso), "Mafioso"},
    {typeid(Sheriff), "Sheriff"},
    {typeid(Doctor), "Doctor"},
    {typeid(Maniac), "Maniac"},
};

Game::Game(size_t n_players, bool human)
    : players(n_players, nullptr)
    , logger(hw1::fs::current_path() / "logs")
    , day{}
{
    size_t cur;

    for (size_t i = 0; i < n_players >> 2; ++i) {
        do {
            cur = random_choice();
        } while (players[cur]);
        players[cur] = hw1::shared_ptr<Player>(new Mafioso);
    }

    do {
        cur = random_choice();
    } while (players[cur]);
    players[cur] = hw1::shared_ptr<Player>(new Sheriff);

    do {
        cur = random_choice();
    } while (players[cur]);
    players[cur] = hw1::shared_ptr<Player>(new Doctor);

    do {
        cur = random_choice();
    } while (players[cur]);
    players[cur] = hw1::shared_ptr<Player>(new Maniac);

    for (cur = 0; cur < n_players; ++cur) {
        if (!players[cur]) {
            players[cur] = hw1::shared_ptr<Player>(new Civilian);
        }
        players[cur]->this_id = cur;
    }

    if (human) {
        cur = random_choice();
        players[cur]->human = true;
        std::cout << "You, Player #" << cur << ", are "
                  << rolenames[typeid(*players[cur])] << ".\n";
    }
}

size_t Game::random_choice(void) const {
    static std::mt19937_64 rng(time(nullptr));
    static std::uniform_int_distribution<size_t> dist(0, players.size() - 1);
    return dist(rng);
}

hw1::async<void> Game::start(void) {
    std::vector<std::pair<size_t, size_t>> results;
    std::unordered_map<size_t, size_t> votes;
    std::vector<hw1::async<std::optional<size_t>>> tasks;
    std::string_view cur_path;

    auto kill_or_cure = [&cur_path, this](
        size_t target,
        bool make_alive,
        const std::string &text
    ) {
        std::string msg = std::format(
            "Player #{} ({}) {}.",
            target, rolenames[typeid(*this->players[target])], text
        );
        players[target]->alive = make_alive;
        std::cout << msg << '\n';
        logger(cur_path, msg);
    };

    while (true) {
        std::cout << ">>>  Day #" << ++day << "  <<<\n";
        cur_path = std::format("day_{}.txt", day);
        results.clear();
        votes.clear();
        tasks.clear();

        for (const auto &pl: players) {
            tasks.push_back(pl->vote(*this));
        }

        for (size_t i = 0; i < tasks.size(); ++i) {
            if (auto vote = co_await tasks[i]; vote.has_value()) {
                ++votes[vote.value()];
            }
        }

        auto day_decision = std::max_element(
            votes.begin(),
            votes.end(),
            [](auto &lhs, auto &rhs) { return lhs.second < rhs.second; }
        );
        kill_or_cure(day_decision->first, false, "got kicked out");

        if (finished()) {
            co_return;
        }

        std::cout << ">>> Night #" << day << " <<<\n";
        cur_path = std::format("night_{}.txt", day);
        results.clear();
        votes.clear();
        tasks.clear();

        for (const auto &pl: players) {
            tasks.push_back(pl->act(*this));
        }

        for (size_t i = 0; i < tasks.size(); ++i) {
            if (auto act = co_await tasks[i]; act.has_value()) {
                results.emplace_back(i, act.value());
            }
        }

        for (const auto &mafia: results | std::views::filter(
            [this](const auto &obj) { return check_role<Mafioso>(obj.first); }
        )) {
            ++votes[mafia.second];
        }
        auto mafia_decision = std::max_element(
            votes.begin(),
            votes.end(),
            [](auto &lhs, auto &rhs) { return lhs.second < rhs.second; }
        );
        if (mafia_decision != votes.end()) {
            kill_or_cure(
                mafia_decision->first, false, "got killed by the mafia"
            );
        }

        auto maniac_decision = results | std::views::filter(
            [this](const auto &obj) { return check_role<Maniac>(obj.first); }
        );
        if (!maniac_decision.empty()) {
            kill_or_cure(
                maniac_decision.front().second, false, "got killed by Maniac"
            );
        }

        auto sheriff_decision = results | std::views::filter(
            [this](const auto &obj) { return check_role<Sheriff>(obj.first); }
        );
        if (!sheriff_decision.empty()) {
            kill_or_cure(
                sheriff_decision.front().second, false, "got killed by Sheriff"
            );
        }

        auto doctor_decision = results | std::views::filter(
            [this](const auto &obj) { return check_role<Doctor>(obj.first); }
        );
        if (!doctor_decision.empty()) {
            kill_or_cure(
                doctor_decision.front().second, true, "got healed by Doctor"
            );
        }

        if (finished()) {
            co_return;
        }
    }
}

bool Game::finished(void) const {
    bool ans = false;
    std::string msg;

    bool maniac_alive = false;
    size_t mafioso_count = 0;
    size_t civilian_count = 0;

    for (const auto &pl: players | std::views::filter(
        [](auto &pl) { return pl->is_alive(); }
    )) {
        if (dynamic_cast<Maniac*>(pl.get())) {
            maniac_alive = true;
        } else if (dynamic_cast<Mafioso*>(pl.get())) {
            ++mafioso_count;
        } else {
            ++civilian_count;
        }
    }

    if (mafioso_count + civilian_count + maniac_alive == 0) {
        msg = "Drawn!";
        ans = true;
    } else if (mafioso_count == 0) {
        if (!maniac_alive) {
            msg = "Civilians win!";
            ans = true;
        } else if (civilian_count <= 1) {
            msg = "Maniac wins!";
            ans = true;
        }
    } else if (!maniac_alive && mafioso_count >= civilian_count) {
        msg = "Mafia wins!";
        ans = true;
    }

    if (ans) {
        std::cout << msg << '\n';
        logger("total.txt", msg);
    }
    return ans;
}

void Game::log(
    const std::string &act, size_t src, size_t dst, bool is_night
) const
{
    logger(
        std::format("{}_{}.txt", is_night ? "night" : "day", day),
        std::format(
            "Player #{} ({}) {} Player #{} ({})",
            src, rolenames[typeid(*players[src])], act,
            dst, rolenames[typeid(*players[dst])]
        )
    );
}

size_t Player::human_input(
    const Game &game,
    const std::string &prompt = "Choose a player to vote against: #"
) {
    size_t result;

    auto cin_reader = [&, this] {
        std::cin >> result;
        if (std::cin.eof()) {
            human = false;
            throw std::string_view(
                "\nEOF caught, leaving control to computer...\n"
            );
        }
        result %= game.n_players();
    };

    std::cout << prompt;
    cin_reader();
    while (!game[result]->is_alive()) {
        std::cout << "This player is dead or kicked out, "
                  << "please choose once again: #";
        cin_reader();
    }
    return result;
}

size_t Player::computer_vote(const Game &game) {
    size_t result;
    do {
        result = game.random_choice();
    } while (!game[result]->is_alive() || game[result] == this);
    return result;
}

hw1::async<std::optional<size_t>> Player::vote(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    if (human) {
        try {
            result = human_input(game);
        } catch (const std::string_view &msg) {
            std::cout << msg;
        }
    }
    if (!human) {
        result = computer_vote(game);
    }
    game.log("votes against", this_id, result, false);
    co_return result;
}

size_t Mafioso::computer_vote(const Game &game) {
    size_t result;
    do {
        result = game.random_choice();
    } while (!game[result]->is_alive() || game.check_role<Mafioso>(result));
    return result;
}

size_t Sheriff::computer_vote(const Game &game) {
    size_t result;
    if (has_target) {
        result = last_checked.value();
    } else {
        do {
            result = game.random_choice();
        } while (
            !game[result]->is_alive()
            || civilians_checked.contains(result)
            || game[result] == this
        );
    }
    return result;
}

hw1::async<std::optional<size_t>> Civilian::act(const Game&) {
    co_return {};
}

hw1::async<std::optional<size_t>> Mafioso::act(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    if (human) {
        try {
            result = human_input(game);
        } catch (const std::string_view &msg) {
            std::cout << msg;
        }
    }
    if (!human) {
        result = computer_vote(game);
    }
    game.log("decides to kill", this_id, result);
    co_return result;
}

hw1::async<std::optional<size_t>> Sheriff::act(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    bool shooting = false;
    if (human) {
        char action{};

        auto cin_reader = [&, this] {
            std::cin >> action;
            if (std::cin.eof()) {
                human = false;
                throw std::string_view(
                    "\nEOF caught, leaving control to computer...\n"
                );
            }
            action = tolower(action);
        };

        try {
            do {
                std::cout << "Would you [c]heck or [s]hoot this time? ";
                cin_reader();
            } while (action != 'c' && action != 's');
            shooting = action == 's';
            result = human_input(game, std::format(
                "Choose a player to {}: #", shooting ? "shoot" : "check"
            ));
        } catch (const std::string_view &msg) {
            std::cout << msg;
        }
    }
    if (!human) {
        if (has_target) {
            result = last_checked.value();
            if (game[result]->is_alive()) {
                shooting = true;
            } else {
                has_target = false;
            }
        }
        if (!has_target) {
            shooting = false;
            do {
                result = game.random_choice();
            } while (
                civilians_checked.contains(result)
                || !game[result]->is_alive()
                || game[result] == this
            );
        }
    }
    game.log(shooting ? "shoots" : "checks", this_id, result);
    if (shooting) {
        co_return result;
    }
    shooting = game.check_role<Mafioso>(result);
    if (human) {
        std::cout << "Player #" << result << " is"
                  << (shooting ? "" : " not") << " a mafioso.\n";
    }
    if (shooting) {
        last_checked = result;
        has_target = true;
    } else {
        civilians_checked.insert(result);
    }
    co_return {};
}

hw1::async<std::optional<size_t>> Doctor::act(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    if (human) {
        try {
            result = human_input(game, "Choose a player to cure: #");
            while (result == last_cured) {
                result = human_input(
                    game,
                    "You cured them last night, please choose once again: #"
                );
            }
        } catch (const std::string_view &msg) {
            std::cout << msg;
        }
    }
    if (!human) {
        do {
            result = game.random_choice();
        } while (!game[result]->is_alive() || result == last_cured);
    }
    last_cured = result;
    game.log("cures", this_id, result);
    co_return result;
}

hw1::async<std::optional<size_t>> Maniac::act(const Game &game) {
    if (!alive) {
        co_return {};
    }
    size_t result;
    if (human) {
        try {
            result = human_input(game, "Choose a player to kill: #");
        } catch (const std::string_view &msg) {
            std::cout << msg;
        }
    }
    if (!human) {
        result = computer_vote(game);
    }
    game.log("kills", this_id, result);
    co_return result;
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
