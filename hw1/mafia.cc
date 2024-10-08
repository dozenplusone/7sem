#include "async.h"
#include "shared_ptr.h"

#include <iostream>
#include <optional>
#include <vector>

class Player;

class Game {
    std::vector<hw1::shared_ptr<Player>> players;
public:
    Game(size_t, bool);
    const Player &player(size_t i) const { return *players[i]; }
    hw1::async<void> start(void);
};

class Player {
protected:
    const bool human;
    bool alive;

public:
    Player(bool is_human): human(is_human), alive(true) {}
    inline bool is_alive(void) const noexcept { return alive; }
    virtual hw1::async<std::optional<size_t>> vote(size_t) = 0;
    virtual hw1::async<std::optional<size_t>> act(size_t) = 0;
};

class Civilian: public Player {
public:
    hw1::async<std::optional<size_t>> vote(size_t) override;
    hw1::async<std::optional<size_t>> act(size_t) override;
};

class Mafioso: public Player {
public:
    hw1::async<std::optional<size_t>> vote(size_t) override;
    hw1::async<std::optional<size_t>> act(size_t) override;
};

class Sheriff: public Civilian {
protected:
    std::optional<size_t> last_checked;
    std::optional<bool> has_shot;

public:
    hw1::async<std::optional<size_t>> vote(size_t) override;
    hw1::async<std::optional<size_t>> act(size_t) override;
};

class Doctor: public Civilian {
protected:
    std::optional<size_t> last_cured;

public:
    hw1::async<std::optional<size_t>> vote(size_t) override;
    hw1::async<std::optional<size_t>> act(size_t) override;
};

class Maniac: public Player {
public:
    hw1::async<std::optional<size_t>> vote(size_t) override;
    hw1::async<std::optional<size_t>> act(size_t) override;
};

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
