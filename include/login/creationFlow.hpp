/*
 * creationFlow.hpp
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "login/characterCreationService.hpp"  // Create::InitialSkills, validators
#include "login/creationSession.hpp"            // CreationSession::Step

struct CharacterDraft;
class Account;

namespace Create::Flow {
    using Step = CreationSession::Step;

    // engine transitions on value, never the rendered letter
    enum class InputKind { None, Letter, Text, Stats, Confirm };

    // Art = raw caret payload; frontend may substitute
    enum class BlockKind { Text, Art };
    struct ViewBlock {
        BlockKind kind = BlockKind::Text;
        std::string text;                 // caret-coded; for Art, the raw payload
        std::vector<std::string> lines;   // Art: pre-split lines (race-menu interleave)
    };

    using ViewOption = Create::Option;

    // options load-bearing; blocks/prompt cosmetic
    struct StepView {
        Step step = Step::Race;
        InputKind inputKind = InputKind::Letter;
        std::string prompt;
        std::vector<ViewBlock> blocks;
        std::vector<ViewOption> options;
    };

    struct Choice {
        Step step = Step::Race;
        std::string value;
    };

    struct StepError {
        bool ok = true;
        std::string code;
        std::string message;
    };

    // Result of advance(): on ok, next is the step to render (or done). granted carries
    // any weapon skills auto-granted on entry, so a frontend can render the
    // "you already know..." line the merged draft can't reconstruct.
    struct AdvanceResult {
        bool ok = false;
        StepError error;
        Step next = Step::Race;
        bool done = false;
        InitialSkills granted;
    };

    // Gate state. EntryContext carries the request's client identity (no Socket).
    // No DM gate here: a DM-owned account may create mortal alts; a DM-name *character*
    // is gated at the Name step (telnet) / rejected there (web), not at entry.
    struct EntryContext {
        std::string ip;
        std::string hostname;
    };
    enum class EntryReason { Ok, LockedOut, LimitReached, ConfigUnavailable, CreationDisabled };
    struct EntryResult {
        bool ok = false;
        EntryReason reason = EntryReason::Ok;
    };

    struct ReplayResult {
        bool ok = false;
        Step failedStep = Step::Race;
        std::string code;
        std::string message;
    };

    // site lockout + account character-limit; no DM gate
    EntryResult checkEntry(const Account& account, const EntryContext& ctx);

    // DM-password sub-flow is telnet-only
    Step firstStep();

    // empty for free-text/confirm steps
    std::vector<ViewOption> options(const CharacterDraft& draft, Step step);

    StepView describe(const CharacterDraft& draft, Step step);

    struct AdvanceOptions {
        const Account* account = nullptr;   // Name step only
        bool checkNameExists = true;        // false: skip the on-disk probe when re-walking a validated path
        bool allowReservedName = false;     // telnet gates a DM name behind the DM password; web rejects
    };
    AdvanceResult advance(CharacterDraft& draft, Step step, const std::string& value,
                          const AdvanceOptions& opts = {});

    // outNext == Done means path complete
    ReplayResult walk(const std::vector<Choice>& path, CharacterDraft& outDraft, Step& outNext,
                      const Account* account = nullptr, bool checkNameExists = true);

    // commit-time re-verification; path must be complete
    ReplayResult replay(const std::vector<Choice>& path, CharacterDraft& outDraft,
                        const Account* account = nullptr);

    // tokens are wire format; renaming breaks clients
    std::string stepToken(Step step);
    std::optional<Step> stepFromToken(const std::string& token);
}
