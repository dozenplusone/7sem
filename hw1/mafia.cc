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
    std::pair<bool, size_t> time;

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

    void log(const std::string&, size_t, size_t) const;
};

class Player {
    friend class Game;

protected:
    std::pair<bool, size_t> time;
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
protected:
    size_t computer_vote(const Game &game) override {
        return Player::computer_vote(game);
    }

public:
    Civilian(bool is_human = false)
        : Player(is_human)
    {}
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

class Mafioso: public Player {
protected:
    size_t computer_vote(const Game&) override;

public:
    Mafioso(bool is_human = false)
        : Player(is_human)
    {}
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

class Bull: public Mafioso {
public:
    Bull(bool is_human = false): Mafioso(is_human) {}
};

class Ninja: public Mafioso {
public:
    Ninja(bool is_human = false): Mafioso(is_human) {}
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

class SheriffLover: public Civilian {
protected:
    std::unordered_set<size_t> checked;
    bool ok;

    size_t computer_vote(const Game&) override;

public:
    SheriffLover(bool is_human = false): Civilian(is_human), ok{} {}
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
protected:
    size_t computer_vote(const Game &game) override {
        return Player::computer_vote(game);
    }

public:
    Maniac(bool is_human = false)
        : Player(is_human)
    {}
    hw1::async<std::optional<size_t>> act(const Game&) override;
};

std::unordered_map<std::type_index, std::string_view> Game::rolenames{
    {typeid(Civilian), "Civilian"},
    {typeid(Mafioso), "Mafioso"},
    {typeid(Bull), "Bull"},
    {typeid(Ninja), "Ninja"},
    {typeid(Sheriff), "Sheriff"},
    {typeid(SheriffLover), "Sheriff's Lover"},
    {typeid(Doctor), "Doctor"},
    {typeid(Maniac), "Maniac"},
};

Game::Game(size_t n_players, bool human)
    : players(n_players, nullptr)
    , logger(hw1::fs::current_path() / "logs")
    , time{}
{
    size_t cur;

    size_t mafioso_count = n_players >> 2;
    size_t others_count = n_players - mafioso_count;

    if (mafioso_count > 1) {
        do {
            cur = random_choice();
        } while (players[cur]);
        players[cur] = hw1::shared_ptr<Player>(new Bull);
        --mafioso_count;
    }

    if (mafioso_count > 1) {
        do {
            cur = random_choice();
        } while (players[cur]);
        players[cur] = hw1::shared_ptr<Player>(new Ninja);
        --mafioso_count;
    }

    for (size_t i = 0; i < mafioso_count; ++i) {
        do {
            cur = random_choice();
        } while (players[cur]);
        players[cur] = hw1::shared_ptr<Player>(new Mafioso);
    }

    do {
        cur = random_choice();
    } while (players[cur]);
    players[cur] = hw1::shared_ptr<Player>(new Sheriff);
    --others_count;

    do {
        cur = random_choice();
    } while (players[cur]);
    players[cur] = hw1::shared_ptr<Player>(new Doctor);
    --others_count;

    do {
        cur = random_choice();
    } while (players[cur]);
    players[cur] = hw1::shared_ptr<Player>(new Maniac);
    --others_count;

    if (others_count > 3) {
        do {
            cur = random_choice();
        } while (players[cur]);
        players[cur] = hw1::shared_ptr<Player>(new SheriffLover);
        --others_count;
    }

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
    static std::mt19937_64 rng(std::time(nullptr));
    static std::uniform_int_distribution<size_t> dist(0, players.size() - 1);
    return dist(rng);
}

hw1::async<void> Game::start(void) {
    std::vector<std::pair<size_t, size_t>> results;
    std::unordered_map<size_t, size_t> votes;
    std::vector<hw1::async<std::optional<size_t>>> tasks;
    std::string_view cur_path;

    auto kill_player = [&cur_path, this](
        size_t target, const std::string &text
    ) {
        std::string msg = std::format(
            "Player #{} ({}) {}.",
            target, rolenames[typeid(*this->players[target])], text
        );
        players[target]->alive = false;
        std::cout << msg << '\n';
        logger(cur_path, msg);
    };

    auto log_who_alive = [&cur_path, this]{
        logger(cur_path, "Players alive:");
        for (auto &pl: players | std::views::filter(
            [](const auto &obj) { return obj->is_alive(); }
        )) {
            logger(cur_path, std::format(
                "#{} ({})",
                pl->this_id, rolenames[typeid(*this->players[pl->this_id])]
            ));
            pl->time = this->time;
        }
    };

    while (true) {
        time.first = false;
        std::cout << ">>>  Day #" << ++time.second << "  <<<\n";
        cur_path = std::format("day_{}.txt", time.second);
        results.clear();
        votes.clear();
        tasks.clear();
        log_who_alive();

        logger(cur_path, "\nActions:");
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
        logger(cur_path, "\nResults:");
        kill_player(day_decision->first, "got kicked out");

        if (finished()) {
            co_return;
        }

        time.first = true;
        std::cout << ">>> Night #" << time.second << " <<<\n";
        cur_path = std::format("night_{}.txt", time.second);
        results.clear();
        votes.clear();
        tasks.clear();
        log_who_alive();

        logger(cur_path, "\nActions:");
        for (const auto &pl: players) {
            tasks.push_back(pl->act(*this));
        }

        for (size_t i = 0; i < tasks.size(); ++i) {
            if (auto act = co_await tasks[i]; act.has_value()) {
                results.emplace_back(i, act.value());
            }
        }
        logger(cur_path, "\nResults:");

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
            kill_player(mafia_decision->first, "got killed by the mafia");
        }

        auto maniac_decision = results | std::views::filter(
            [this](const auto &obj) { return check_role<Maniac>(obj.first); }
        );
        if (!maniac_decision.empty()) {
            size_t target = maniac_decision.front().second;
            if (!check_role<Bull>(target)) {
                kill_player(target, "got killed by Maniac");
            }
        }

        auto sheriff_decision = results | std::views::filter(
            [this](const auto &obj) { return check_role<Sheriff>(obj.first); }
        );
        if (!sheriff_decision.empty()) {
            kill_player(
                sheriff_decision.front().second, "got killed by Sheriff"
            );
        }

        auto doctor_decision = results | std::views::filter(
            [this](const auto &obj) { return check_role<Doctor>(obj.first); }
        );
        if (!doctor_decision.empty()) {
            size_t target = doctor_decision.front().second;
            players[target]->alive = true;
            std::cout << "Player #" << target << " got healed by Doctor.\n";
            logger(cur_path, std::format(
                "Player #{} ({}) got healed by Doctor.",
                target, rolenames[typeid(*players[target])]
            ));
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
        std::string_view cur_path = "total.txt";
        logger(cur_path, "Players:");
        for (auto &pl: players) {
            logger(cur_path, std::format(
                "#{}: {}{}, {}",
                pl->this_id,
                rolenames[typeid(*players[pl->this_id])],
                pl->human ? ", human-driven" : "",
                pl->alive ? "survived" : std::format(
                    "{} at {} {}",
                    pl->time.first ? "killed" : "kicked out",
                    pl->time.first ? "Night" : "Day",
                    pl->time.second
                )
            ));
        }

        std::cout << msg << '\n';
        logger(cur_path, "\nResults:");
        logger(cur_path, msg);
    }
    return ans;
}

void Game::log(const std::string &act, size_t src, size_t dst) const {
    logger(
        std::format("{}_{}.txt", time.first ? "night" : "day", time.second),
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
    game.log("votes against", this_id, result);
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

size_t SheriffLover::computer_vote(const Game &game) {
    size_t result;
    do {
        result = game.random_choice();
    } while (
        !game[result]->is_alive() 
        || game.check_role<Sheriff>(result)
        || game[result] == this
    );
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
    shooting = game.check_role<Mafioso>(result)
                && !game.check_role<Ninja>(result);
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

hw1::async<std::optional<size_t>> SheriffLover::act(const Game &game) {
    if (alive && !ok) {
        size_t result;
        if (human) {
            try {
                result = human_input(game, "Choose a player to check: #");
            } catch (const std::string_view &msg) {
                std::cout << msg;
            }
        }
        if (!human) {
            do {
                result = game.random_choice();
            } while (
                checked.contains(result)
                || !game[result]->is_alive()
                || game[result] == this
            );
        }
        ok = game.check_role<Sheriff>(result);
        if (!ok) {
            checked.insert(result);
        }
        if (human) {
            std::cout << "Player #" << result << " is"
                      << (ok ? "" : " not") << " Sheriff.\n";
        }
        game.log("checks", this_id, result);
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
    game.log("shoots", this_id, result);
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
