#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "PCH.h"

namespace LL {

    // One scene position; the SexLab counts cover history the ledger may not hold yet
    struct FirstTimePosition {
        RE::Actor* actor{nullptr};
        int slTotal{0};
        int slVaginal{0};
        int slAnal{0};
        int anatomy{-1};  // SexLab anatomy (kAnatomyPenis | kAnatomyVagina); -1 = unknown, use the base sex
    };

    // One line per unique position whose scene is a first (sex at all, a partner gender / race, a group size,
    // giving or receiving vaginal / anal), newline-separated; "" when nothing is new.
    // tags: the animation's comma-separated SexLab tags; position 0 receives Vaginal / Anal.
    std::string DescribeFirstTimes(const std::vector<FirstTimePosition>& positions, std::string_view tags);

}  // namespace LL
