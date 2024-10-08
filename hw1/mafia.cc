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
    virtual hw1::async<std::optional<size_t>> vote(size_t) = 0;
    virtual hw1::async<std::optional<size_t>> act(size_t) = 0;
};

class Civilian: public Player {
public:
    Civilian(bool is_human = false)
        : Player(is_human)
    {}
    hw1::async<std::optional<size_t>> vote(size_t) override;
    hw1::async<std::optional<size_t>> act(size_t) override;
};

class Mafioso: public Player {
public:
    Mafioso(bool is_human = false)
        : Player(is_human)
    {}
    hw1::async<std::optional<size_t>> vote(size_t) override;
    hw1::async<std::optional<size_t>> act(size_t) override;
};

class Sheriff: public Civilian {
protected:
    std::optional<size_t> last_checked;
    std::optional<bool> has_shot;

public:
    Sheriff(bool is_human = false)
        : Civilian(is_human)
        , last_checked{}
        , has_shot{}
    {}
    hw1::async<std::optional<size_t>> vote(size_t) override;
    hw1::async<std::optional<size_t>> act(size_t) override;
};

class Doctor: public Civilian {
protected:
    std::optional<size_t> last_cured;

public:
    Doctor(bool is_human = false)
        : Civilian(is_human)
        , last_cured{}
    {}
    hw1::async<std::optional<size_t>> vote(size_t) override;
    hw1::async<std::optional<size_t>> act(size_t) override;
};

class Maniac: public Player {
public:
    Maniac(bool is_human = false)
        : Player(is_human)
    {}
    hw1::async<std::optional<size_t>> vote(size_t) override;
    hw1::async<std::optional<size_t>> act(size_t) override;
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
    co_return;
}

int main(int argc, const char *argv[]) {
    size_t n_players = 0;
    if (argc > 1) {
        n_players = strtoul(argv[1], nullptr, 10);
    }
    while (n_players < 5) {
        std::cin >> n_players;
    }
    bool human = std::getchar() == 'Y';
    Game game(n_players, human);
    game.start();
    return 0;
}
