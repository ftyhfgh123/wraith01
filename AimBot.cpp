    struct AimBot {
        ConfigLoader* cl;
        XDisplay* display;
        Level* level;
        LocalPlayer* localPlayer;
        std::vector<Player*>* players;
        Player* target = nullptr;
     
        AimBot(ConfigLoader* cl, XDisplay* display, Level* level, LocalPlayer* localPlayer, std::vector<Player*>* players) {
            this->cl = cl;
            this->display = display;
            this->level = level;
            this->localPlayer = localPlayer;
            this->players = players;
        }
     
        void aimAssist(int counter) {
            if (!active()) {
                releaseTarget();
                return;
            }
            if (target == nullptr)
                assignTarget();
            if (target == nullptr)
                return;
     
            // Check if the target is within 8 meters range
            bool withinRange = target->distance2DToLocalPlayer < util::metersToGameUnits(8);
     
            if (withinRange) {
                // Within 8 meters, always lock onto the target
                moveMouseWithShaking();
                return;
            }
     
            // Beyond 8 meters
            if (!target->visible ||
                fabs(target->aimbotDesiredAnglesIncrement.x) > cl->AIMBOT_FOV ||
                fabs(target->aimbotDesiredAnglesIncrement.y) > cl->AIMBOT_FOV) {
                // Release the lock if the target is not visible or outside the FOV
                releaseTarget();
                return;
            }
     
            // Continue tracking the target outside 8 meters if it's within FOV
            moveMouseWithRandomization();
        }
     
        void moveMouseWithRandomization() {
            //calculate smoothing    
            float EXTRA_SMOOTH = cl->AIMBOT_SMOOTH_EXTRA_BY_DISTANCE / target->distanceToLocalPlayer;
            float TOTAL_SMOOTH = cl->AIMBOT_SMOOTH + EXTRA_SMOOTH;
     
            // Introduce randomization for more human-like behavior
            float randomSpeedFactor = 1.0 + (rand() % 50 - 5) / 100.0; // Random speed shift
            float randomReactionTime = (rand() % 150) / 250.0; // Random reaction time between 0.15 and 0.3 seconds
     
            TOTAL_SMOOTH *= randomSpeedFactor;
     
            // Introduce random delay before moving the mouse
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(randomReactionTime * 100)));
     
            // Call the original moveMouse function
            moveMouseWithShaking();
        }
     
    void moveMouseWithShaking() {
        // Calculate smoothing f   
        float EXTRA_SMOOTH = cl->AIMBOT_SMOOTH_EXTRA_BY_DISTANCE / target->distanceToLocalPlayer;
        float TOTAL_SMOOTH = cl->AIMBOT_SMOOTH + EXTRA_SMOOTH;
        // Aimbot calcs
        const FloatVector2D aimbotDelta = target->aimbotDesiredAnglesIncrement
            .multiply(100)
            .divide(TOTAL_SMOOTH);
     
        const double aimYawIncrement = aimbotDelta.y * -1;
        const double aimPitchIncrement = aimbotDelta.x;
        // Combine
        const double totalPitchIncrement = aimPitchIncrement;
        const double totalYawIncrement = aimYawIncrement;
     
        // Introduce shaking effect
        const int shakeRange = 2; // Define the range of shaking, adjust as needed
        int shakePitch = (rand() % (2 * shakeRange + 1)) - shakeRange;
        int shakeYaw = (rand() % (2 * shakeRange + 1)) - shakeRange;
     
        // Turn into integers
        int totalPitchIncrementInt = roundHalfEven(atLeast_1_AwayFromZero(totalPitchIncrement + shakePitch));
        int totalYawIncrementInt = roundHalfEven(atLeast_1_AwayFromZero(totalYawIncrement + shakeYaw));
     
        // Deadzone check
        if (fabs(target->aimbotDesiredAnglesIncrement.x) < cl->AIMBOT_DEADZONE) totalPitchIncrementInt = 0;
        if (fabs(target->aimbotDesiredAnglesIncrement.y) < cl->AIMBOT_DEADZONE) totalYawIncrementInt = 0;
        if (totalPitchIncrementInt == 0 && totalYawIncrementInt == 0) return;
     
        // Move mouse with shaking effect
        display->moveMouseRelative(totalPitchIncrementInt, totalYawIncrementInt);
    }
     
        bool active() {
            bool aimbotIsOn = cl->FEATURE_AIMBOT_ON;
            bool combatReady = localPlayer->isCombatReady();
            bool activatedByAttackingAndIsAttacking = cl->AIMBOT_ACTIVATED_BY_ATTACK && localPlayer->inAttack;
            bool activatedByADSAndIsADSing = cl->AIMBOT_ACTIVATED_BY_ADS && localPlayer->inZoom;
            bool activatedByButtonAndButtonIsDown = cl->AIMBOT_ACTIVATED_BY_BUTTON != "" && display->keyDown(cl->AIMBOT_ACTIVATED_BY_BUTTON);
            bool active = aimbotIsOn
                && combatReady
                && (activatedByAttackingAndIsAttacking
                    || activatedByADSAndIsADSing
                    || activatedByButtonAndButtonIsDown);
            return active;
        }
     
        void assignTarget() {
            for (int i = 0;i < players->size();i++) {
                Player* p = players->at(i);
                if (!p->isCombatReady())continue;
                if (!p->enemy) continue;
                if (!p->visible) continue;
                if (p->aimedAt) continue;
                if (fabs(p->aimbotDesiredAnglesIncrement.x) > cl->AIMBOT_FOV) continue;
                if (fabs(p->aimbotDesiredAnglesIncrement.y) > cl->AIMBOT_FOV) continue;
                if (target == nullptr || p->aimbotScore > target->aimbotScore) {
                    target = p;
                    //                target->aimbotLocked = true;
                }
            }
        }
     
        void releaseTarget() {
            if (target != nullptr && target->isValid())
                target->aimbotLocked = false;
            target = nullptr;
        }
     
        void resetLockFlag() {
            for (int i = 0;i < players->size();i++) {
                Player* p = players->at(i);
                if (!p->isCombatReady()) continue;
                p->aimbotLocked = false;
            }
            if (target != nullptr)
                target->aimbotLocked = true;
        }
     
        int roundHalfEven(float x) {
            return (x >= 0.0)
                ? static_cast<int>(std::round(x))
                : static_cast<int>(std::round(-x)) * -1;
        }
     
        float atLeast_1_AwayFromZero(float num) {
            if (num > 0) return std::max(num, 1.0f);
            return std::min(num, -1.0f);
        }
    };
