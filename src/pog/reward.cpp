// Copyright (c) 2017 The Merit Foundation developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pog/reward.h"

#include <algorithm>
#include <numeric>

namespace pog
{
    const int DAY = 24 * 60 * 60;

    AmbassadorLottery RewardAmbassadors(
            int height,
            const referral::AddressANVs& winners,
            CAmount total_reward)
    {
        /**
         * Increase ANV precision on block 16000
         */
        CAmount fixed_precision = height < 16000 ? 100 : 1000;

        CAmount total_anv =
            std::accumulate(std::begin(winners), std::end(winners), CAmount{0},
                    [](CAmount acc, const referral::AddressANV& v)
                    {
                        return acc + v.anv;
                    });

        Rewards rewards(winners.size());
        std::transform(std::begin(winners), std::end(winners), std::begin(rewards),
                [total_reward, total_anv, fixed_precision](const referral::AddressANV& v)
                {
                    double percent = (v.anv*fixed_precision) / total_anv;
                    CAmount reward = (total_reward * percent) / fixed_precision;
                    assert(reward <= total_reward);
                    return AmbassadorReward{v.address_type, v.address, reward};
                });

        Rewards filtered_rewards;
        filtered_rewards.reserve(rewards.size());
        std::copy_if(std::begin(rewards), std::end(rewards),
                std::back_inserter(filtered_rewards),
                [](const AmbassadorReward& reward) {
                    return reward.amount > 0;
                });

        CAmount total_rewarded =
            std::accumulate(std::begin(filtered_rewards), std::end(filtered_rewards), CAmount{0},
                    [](CAmount acc, const AmbassadorReward& reward)
                    {
                        return acc + reward.amount;
                    });

        assert(total_rewarded >= 0);
        assert(total_rewarded <= total_reward);

        auto remainder = total_reward - total_rewarded;

        assert(remainder >= 0);
        assert(remainder <= total_reward);

        return {filtered_rewards, remainder};
    }

    int ComputeTotalInviteLotteryWinners(
            int height,
            const InviteLotteryParams& lottery,
            const Consensus::Params& params)
    {

        const auto period = (height - params.vDeployments[Consensus::DEPLOYMENT_DAEDALUS].start_block) /
            params.daedalus_block_window;

        if(period < 1) {
            return params.daedalus_max_invites_per_block;
        }

        assert(lottery.invites_used >= 0);

        if(lottery.invites_used == 0) {
            /**
             * If no invites are generated and no invites are used,
             * There is a chance those that use invites are starved and
             * those that don't use invites have too many. 
             *
             * Create an invite in the hope of giving it to someone who will
             * use it.
             */
            if(lottery.invites_created == 0) {
                return 1;
            } else {
                return 0;
            }
        }

        const auto scaled_invites_used = lottery.invites_used * 100;

        const auto velocity = lottery.invites_created > 0 ? 
            std::min(scaled_invites_used / lottery.invites_created, 100) : 100;

        const int total_winners = (params.daedalus_max_invites_per_block * velocity) / 100 ;

        assert(total_winners >= 0 && total_winners <= params.daedalus_max_invites_per_block);
        return total_winners;
    }

    InviteRewards RewardInvites(const referral::ConfirmedAddresses& winners)
    {
        assert(winners.size() >= 0);

        const auto INVITES_PER_WINNER = 1;

        InviteRewards rewards(winners.size());
        std::transform(winners.begin(), winners.end(), rewards.begin(),
                [INVITES_PER_WINNER](const referral::ConfirmedAddress& winner) -> InviteReward {
                    return {
                        winner.address_type,
                        winner.address,
                        INVITES_PER_WINNER
                    };
                });

        assert(rewards.size() == winners.size());
        return rewards;
    }

} // namespace pog
