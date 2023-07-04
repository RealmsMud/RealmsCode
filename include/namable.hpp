//
// Created by jason on 9/18/21.
//

#pragma once

// A common class that has a name and description to avoid two separate classes with name/desc (skill & command) being inherited by SkillCommand
class Nameable {
public:
    Nameable() = default;
    virtual ~Nameable() = default;

    std::string name{};
    std::string description{};

    [[nodiscard]] std::string getName() const;
    [[nodiscard]] std::string getDescription() const;

};

