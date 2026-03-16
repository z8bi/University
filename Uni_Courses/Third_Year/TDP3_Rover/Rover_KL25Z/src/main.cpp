    #include "mbed.h"

    #include "pin_definitions.hpp"
    #include "config.hpp"
    #include "types.hpp"
    #include "sensors.hpp"
    #include "motors.hpp"
    #include "ultrasonic_sensor.hpp"
    #include "tcs3472.hpp"
    #include "debug.hpp"

    // #include "audio.hpp"
    // #include "sounds.hpp"

    using namespace std::chrono_literals;

    //======================================================
    //================== TIMERS / FLAGS ====================
    //======================================================

    namespace Flags {
    Ticker debugTick;
    Ticker controlTick;
    Ticker ultraTick;
    Ticker colorTick;

    volatile bool control_due = false;
    volatile bool ultra_due   = false;
    volatile bool color_due   = false;
    volatile bool debug_due   = false;

    static void control_isr() { control_due = true; }
    static void ultra_isr()   { ultra_due   = true; }
    static void color_isr()   { color_due   = true; }
    static void debug_isr()   { debug_due   = true; }

    } // namespace Flags

    //======================================================
    //================= CONTROLLER RUNTIME =================
    //======================================================

    //Obstacle-avoid sub states
    enum class ObPhase { MOVE_CLOSER, TURN_LEFT_45, SEARCH_RIGHT_WALL, REACQUIRE }; 

    //all statics of controller runtime - stored in variable rt to be used in controller update and obstacle avoid logic
    struct ControllerRuntime {
        CtrlState prev_state = CtrlState::FOLLOW;

        bool ob_reset_pending = false;
        bool ob_active        = false;

        int red_ctrl_cnt   = 0;
        int ob_cnt         = 0;
        int found_cnt      = 0;
        int lost_cnt       = 0;
        int align_lost_cnt = 0;

        // SEEK center-lock
        bool seek_lock_active = false;
        bool seek_lock_enabled = false;   // only allow middle-lock when FOLLOW triggered SEEK
        int  seek_lock_dir    = 0; // -1 left, +1 right
        Kernel::Clock::time_point seek_lock_until{};
        bool ob_seek_force_full_duration = false;

        // Obstacle-avoid sub-state
        ObPhase phase = ObPhase::TURN_LEFT_45;
        Kernel::Clock::time_point phase_until{};
        Kernel::Clock::time_point sub_brake_until{};
        Kernel::Clock::time_point forward_min_until{};
        bool  did_follow       = false;
        bool reacq_has_seen = false;
        int   line_found_cnt   = 0;
        float target_right_distance = Config::RIGHT_MAX_CM;
        int right_counter = 0;
        bool ob_left_break_active = false;
        Kernel::Clock::time_point ob_left_break_until{};

        // STOPPED sub-state
        enum class StopPhase { HOLD_ON_RED, BACKUP_FIND_RED };
        StopPhase stop_phase = StopPhase::HOLD_ON_RED;
        int stop_no_red_cnt = 0;
        int stop_red_reacq_cnt = 0;
        int stop_green_cnt = 0;
    };

    //initialize all static tracking variables
    static ControllerRuntime rt;

    //======================================================
    //==================== SHARED DATA =====================
    //======================================================

    struct Shared {
        // Color
        tcs3472::RGBC rgbc{};
        bool          rgbc_valid = false;
        LightState    light      = LightState::NONE;

        // Ultrasonic
        float front_cm = -1.0f;
        bool  front_ok = false;
        Kernel::Clock::time_point front_stamp{};
        UltrasonicSensor::Reading::State front_state = UltrasonicSensor::Reading::State::NO_ECHO;

        float right_cm = -1.0f;
        bool  right_ok = false;
        Kernel::Clock::time_point right_stamp{};
        UltrasonicSensor::Reading::State right_state = UltrasonicSensor::Reading::State::NO_ECHO;
    };

    static Shared g_shared;

    static inline Shared shared_snapshot() {
        Shared sh;
        core_util_critical_section_enter();
        sh = g_shared;
        core_util_critical_section_exit();
        return sh;
    }

    static inline void shared_set_front(float cm, bool ok) {
        core_util_critical_section_enter();
        g_shared.front_cm    = ok ? cm : -1.0f;
        g_shared.front_ok    = ok;
        g_shared.front_stamp = Kernel::Clock::now();
        core_util_critical_section_exit();
    }

    static inline void shared_set_right(float cm, bool ok) {
        core_util_critical_section_enter();
        g_shared.right_cm    = ok ? cm : -1.0f;
        g_shared.right_ok    = ok;
        g_shared.right_stamp = Kernel::Clock::now();
        core_util_critical_section_exit();
    }

    static inline void shared_set_color(const tcs3472::RGBC& v, bool valid, LightState ls) {
        core_util_critical_section_enter();
        g_shared.rgbc       = v;
        g_shared.rgbc_valid = valid;
        g_shared.light      = ls;
        core_util_critical_section_exit();
    }

    //======================================================
    //==================== STATE / MODE ====================
    //======================================================

    static CtrlState state = CtrlState::FOLLOW;
    static Kernel::Clock::time_point state_until{};
    static Kernel::Clock::time_point stop_brake_until{};

    static int  last_pos     = 0;   // -1 left, +1 right
    static bool manual_mode  = false;
    static bool found_light_again = false;
    static char bt_char      = 0;

    //======================================================
    //==================== HELPERS =========================
    //======================================================

    static inline bool bt_connected() {
        return HW::bt_state.read() == 1;
    }

    static inline void leds_set(bool r, bool g, bool b) {
        HW::LED_R = !r;
        HW::LED_G = !g;
        HW::LED_B = !b;
    }

    static inline bool debounce(bool cond, int& counter, int threshold) {
        counter = cond ? (counter + 1) : 0;
        return counter >= threshold;
    }

    static void leds_for_state(CtrlState st) {
        switch (st) {
            case CtrlState::FOLLOW:         leds_set(false, true,  false); break;
            case CtrlState::SEEK_LEFT:      leds_set(true,  false, false); break;
            case CtrlState::SEEK_RIGHT:     leds_set(true,  false, false); break;
            case CtrlState::ALIGN:          leds_set(true,  true,  false); break;
            case CtrlState::BRAKING:        leds_set(false, false, true ); break;
            case CtrlState::OBSTACLE_AVOID: leds_set(false, true,  true ); break;
            case CtrlState::STOPPED:        leds_set(true,  false, true ); break;
            case CtrlState::FULLY_STOPPED:  leds_set(true,  true,  true ); break;
            case CtrlState::MANUAL_BT:      /* driven by command LEDs */   break;
            default:                        leds_set(false, false, false); break;
        }
    }

    static void leds_for_manual_cmd(char cmd) {
        switch (cmd) {
            case 'W': leds_set(false, true,  false); break; // GREEN
            case 'S': leds_set(true,  false, true ); break; // MAGENTA
            case 'A': leds_set(false, false, true ); break; // BLUE
            case 'D': leds_set(true,  false, false); break; // RED
            case 'Z': leds_set(true,  true, false); break; // YELLOW
            default:  break;
        }
    }

    static void enter_state(CtrlState st, int duration_ms) {
        core_util_critical_section_enter();
        state       = st;
        state_until = Kernel::Clock::now() + std::chrono::milliseconds(duration_ms);
        core_util_critical_section_exit();

        leds_for_state(st);
        Debug::update_state(st);
    }

    static inline bool state_time_elapsed() {
        return Kernel::Clock::now() >= state_until;
    }

    static LightState classify_light(const tcs3472::RGBC& v) {
        if (!v.valid || v.c < Config::COLOR_INTENSITY) return LightState::NONE;

        const int rP = (int)((100U * v.r) / v.c);
        const int gP = (int)((100U * v.g) / v.c);

        if ((rP - gP) >= 7 && rP >= 40) return LightState::RED;
        if ((gP - rP) >= 5 && gP >= 38) return LightState::GREEN;

        return LightState::NONE;
    }

    static inline void cancel_obstacle_mode() {
        core_util_critical_section_enter();
        rt.ob_active = false;
        rt.ob_reset_pending = true; // so next OB entry starts clean
        core_util_critical_section_exit();
    }

    //======================================================
    //================== CONTROLLER UPDATE =================
    //======================================================

    static void controller_update() {
        Sensors  s  = read_sensors();
        LineInfo li = interpret(s);

        Debug::update_line(Debug::make_line(s, li, true));

        const Shared sh = shared_snapshot();
        const LightState ls = sh.light;

        // exit if FULLY STOPPED or MANUAL
        if (manual_mode && state != CtrlState::FULLY_STOPPED) return;

        // Track state changes
        if (state != rt.prev_state) {
            if (state == CtrlState::OBSTACLE_AVOID) {
                rt.ob_reset_pending = true; //make sure to reset all OB statics once obstacle avoid is exited
            }

            if (state == CtrlState::STOPPED) {
                rt.stop_phase = ControllerRuntime::StopPhase::HOLD_ON_RED;
                rt.stop_no_red_cnt = 0;
                rt.stop_red_reacq_cnt = 0;
                rt.stop_green_cnt = 0;
            }

            rt.prev_state = state;
        }

        // If obstacle mode was latched, force it
        if (rt.ob_active && state != CtrlState::OBSTACLE_AVOID && manual_mode == false) {
            enter_state(CtrlState::OBSTACLE_AVOID, 0);
        }

        const bool lock_ob = rt.ob_active;

        // Debounced event flags
        bool red_confirmed         = false;
        bool obstacle_confirmed    = false;

        // Only allow new transitions when obstacle mode is not locked -> this way when obstacle is triggered it won't be overwritted until it releases control
        if (!lock_ob) {

            // ----- FRONT OBSTACLE -> OBSTACLE_AVOID -----
            const bool front_fresh = (Kernel::Clock::now() - sh.front_stamp) < 300ms;
            const bool front_hit   = sh.front_ok && front_fresh &&
                                    sh.front_cm > 0.0f && sh.front_cm < Config::OB_FRONT_TRIG_CM;

            const bool right_fresh = (Kernel::Clock::now() - sh.right_stamp) < 300ms;
            const bool right_present = sh.right_ok && right_fresh &&
                                    sh.right_cm > 0.0f && sh.right_cm < Config::RIGHT_MAX_CM;

            const bool ob_gate = (state == CtrlState::FOLLOW || state == CtrlState::ALIGN);

            obstacle_confirmed = debounce(ob_gate && front_hit, rt.ob_cnt, Config::OB_CONFIRM_CTRL_TICKS);

            if (ob_gate && obstacle_confirmed) {
                rt.ob_active        = true;
                rt.ob_reset_pending = true;

                rt.red_ctrl_cnt   = 0;
                rt.lost_cnt       = 0;
                rt.align_lost_cnt = 0;
                rt.found_cnt      = 0;
                rt.ob_cnt         = 0;

                enter_state(CtrlState::OBSTACLE_AVOID, Config::OB_BRAKE_MS);
                motors_brake(Config::BRAKE_STRENGTH);
                return;
            }

            // ----- RED -> STOPPED -----
            red_confirmed = debounce(ls == LightState::RED, rt.red_ctrl_cnt, Config::RED_CTRL_CONFIRM_TICKS);

            if (state != CtrlState::FULLY_STOPPED &&
                state != CtrlState::STOPPED &&
                red_confirmed &&
                right_present) // ensure the traffic light is actually there to avoid red lockers doing stupid things
            {
                rt.red_ctrl_cnt = 0;
                enter_state(CtrlState::STOPPED, 0);
                stop_brake_until = Kernel::Clock::now() + std::chrono::milliseconds(Config::STOP_BRAKE_MS);
                motors_brake(Config::BRAKE_STRENGTH);
                return;
            }

            // Track last-known direction
            if (!li.lost) {
                if (li.pos > 0.2f)       last_pos = +1;
                else if (li.pos < -0.2f) last_pos = -1;
            }

            // SEEK center-lock housekeeping
            if (state != CtrlState::SEEK_LEFT && state != CtrlState::SEEK_RIGHT) {
                rt.seek_lock_active = false;
                rt.seek_lock_enabled = false;
                rt.seek_lock_dir    = 0;
                rt.ob_seek_force_full_duration = false;
            }
        }

        //============================== FSM ==============================

        /*---------------------------------------------------- 
        FOLLOW - DRIVES IN A STRAIGHT LINE IF CENTERED, ATTEMPTS SLIGHT SMOOTH TURNS --> FALLS BACK TO ALIGN FOR BIGGER MISALIGNMENTS
        ALIGN - USES SMOOTH TURNING TO MORE AGGRESIVELY ATTEMPT TO CENTER --> FALLS BACK TO SEEK IF LINE IS LOST
        SEEK - USES SKID STEERING TO TURN IN A DIRECTION BASED ON THE LAST KNOWN DIRECTION UNTIL THE LINE IS FOUND AGAIN, ATTEMPTS TO FIND CENTER BUT GOES TO ALIGN IF CENTER NOT FOUND IN TIME LIMIT SET BY CONFIG
        BRAKING - OBVIOUS
        OBSTACLE_AVOID - TRIGGERED WHEN FRONT SENSOR SPOTS OBJECT, TURNS LEFT AND USES THE RIGHT SENSOR TO MANUEVER AROUND THE OBJECT UNTIL THE LINE IS FOUND AGAIN
        STOPPED - TRIGGERED WHEN THE COLOR SENSOR SPOTS THE RED TRAFFIC LIGHT, RELEASED WHEN GREEN IS SPOTTED. IF RED LIGHT IS LOST (ROVER HADN'T BRAKED FOR LONG ENOUGH AND WENT PAST), REVERSE UNTIL RED LIGHT IS SPOTTED AGAIN
        FULLY_STOPPED - DEFAULT STATE AT THE START SO BLUETOOTH CAN BE PAIRED, ALSO TRIGGERED BY MANUAL CONTROL AND CAN ONLY BE RELEASED BY THE USER OVER BLUETOOTH 
        ----------------------------------------------------*/

        switch (state) {

            case CtrlState::FOLLOW: {
                if (li.right_turn_sig && !li.left_turn_sig) {
                    last_pos = +1;
                    enter_state(CtrlState::BRAKING, Config::BRAKE_MS);
                    motors_brake(Config::BRAKE_STRENGTH);
                    break;
                }

                if (li.left_turn_sig && !li.right_turn_sig) {
                    last_pos = -1;
                    enter_state(CtrlState::BRAKING, Config::BRAKE_MS);
                    motors_brake(Config::BRAKE_STRENGTH);
                    break;
                }

                if (li.centered) {
                    move_forward(Config::DUTY_FWD);
                    break;
                }

                enter_state(CtrlState::ALIGN, 0);
                break;
            }

           case CtrlState::ALIGN: {
                if (li.centered) {
                    enter_state(CtrlState::FOLLOW, 0);
                    move_forward(Config::DUTY_FWD);
                    break;
                }

                if (li.right_turn_sig && !li.left_turn_sig) {
                    last_pos = +1;
                    enter_state(CtrlState::BRAKING, Config::BRAKE_MS);
                    motors_brake(Config::BRAKE_STRENGTH);
                    break;
                }

                if (li.left_turn_sig && !li.right_turn_sig) {
                    last_pos = -1;
                    enter_state(CtrlState::BRAKING, Config::BRAKE_MS);
                    motors_brake(Config::BRAKE_STRENGTH);
                    break;
                }

                if (li.lost) {
                    rt.seek_lock_enabled = false;
                    enter_state((last_pos >= 0) ? CtrlState::SEEK_RIGHT : CtrlState::SEEK_LEFT, 0);
                    break;
                }

                if (li.pos > 0.0f) turn_right_break_inner(Config::TURN_BRAKE_STRENGTH, Config::ALIGN_DUTY_TURN);
                else               turn_left_break_inner (Config::TURN_BRAKE_STRENGTH, Config::ALIGN_DUTY_TURN);
                break;
            }

            case CtrlState::SEEK_LEFT: {
                // Special obstacle-avoid handoff:
                // after finding the line, do a short left brake-inner turn first
                if (rt.ob_left_break_active) {
                    if (Kernel::Clock::now() < rt.ob_left_break_until) {
                        turn_left_break_inner(Config::TURN_BRAKE_STRENGTH, Config::ALIGN_DUTY_TURN);
                        break;
                    } else {
                        rt.ob_left_break_active = false;
                    }
                }

                if (rt.seek_lock_active && rt.seek_lock_dir != -1) { rt.seek_lock_active = false; rt.seek_lock_dir = 0; }

                if (rt.seek_lock_enabled && !rt.seek_lock_active && s.on[0]) {
                    rt.seek_lock_active = true;
                    rt.seek_lock_dir    = -1;
                    rt.seek_lock_until  = Kernel::Clock::now() + std::chrono::milliseconds(Config::SEEK_CENTER_LOCK_MS);
                }

                if (rt.seek_lock_active && rt.seek_lock_dir == -1) {
                    if (li.centered && !rt.ob_seek_force_full_duration) {
                        rt.seek_lock_active = false;
                        enter_state(CtrlState::FOLLOW, 0);
                        break;
                    }
                    if (Kernel::Clock::now() < rt.seek_lock_until) {
                        turn_left_skid_reverse_inner(Config::DUTY_TURN);
                        break;
                    }
                    rt.seek_lock_active = false;
                    rt.ob_seek_force_full_duration = false;
                }

                if (!li.fully_lost) {
                    enter_state(CtrlState::ALIGN, 0);
                    break;
                }

                turn_left_skid_reverse_inner(Config::DUTY_TURN);
                break;
            }

            case CtrlState::SEEK_RIGHT: {
                if (rt.seek_lock_active && rt.seek_lock_dir != +1) { rt.seek_lock_active = false; rt.seek_lock_dir = 0; }

                if (rt.seek_lock_enabled && !rt.seek_lock_active && s.on[4]) {
                    rt.seek_lock_active = true;
                    rt.seek_lock_dir    = +1;
                    rt.seek_lock_until  = Kernel::Clock::now() + std::chrono::milliseconds(Config::SEEK_CENTER_LOCK_MS);
                }

                if (rt.seek_lock_active && rt.seek_lock_dir == +1) {
                    if (li.centered) {
                        rt.seek_lock_active = false;
                        enter_state(CtrlState::FOLLOW, 0);
                        break;
                    }
                    if (Kernel::Clock::now() < rt.seek_lock_until) {
                        turn_right_skid_reverse_inner(Config::DUTY_TURN);
                        break;
                    }
                    rt.seek_lock_active = false;
                }

                if (!li.fully_lost) {
                    enter_state(CtrlState::ALIGN, 0);
                    break;
                }

                turn_right_skid_reverse_inner(Config::DUTY_TURN);
                break;
            }
            
            case CtrlState::BRAKING: {
                if (state_time_elapsed()) {
                    motors_all_off();
                    rt.seek_lock_enabled = true;   // FOLLOW triggered this SEEK
                    enter_state((last_pos >= 0) ? CtrlState::SEEK_RIGHT : CtrlState::SEEK_LEFT, 0);
                } else {
                    motors_brake(Config::BRAKE_STRENGTH);
                }
                break;
            }

            case CtrlState::OBSTACLE_AVOID: {
                // Initial brake window
                if (!state_time_elapsed()) {
                    motors_brake(Config::BRAKE_STRENGTH);
                    break;
                }

                auto leds_for_ob_phase = [](ObPhase p) {
                    switch (p) {
                        case ObPhase::MOVE_CLOSER:       leds_set(false,  true, false); break; // GREEN
                        case ObPhase::TURN_LEFT_45:      leds_set(true,  false, true); break; // PURPLE
                        case ObPhase::SEARCH_RIGHT_WALL: leds_set(false, false, true ); break; // BLUE
                        case ObPhase::REACQUIRE:         leds_set(true,  true,  false); break; // YELLOW
                        default:                         leds_set(false, true,  true ); break;
                    }
                };

                // Reset when re-entering
                if (rt.ob_reset_pending) {
                    rt.ob_reset_pending = false;
                    rt.did_follow       = false;
                    rt.reacq_has_seen = false;
                    rt.phase            = ObPhase::MOVE_CLOSER;
                    rt.phase_until      = {};
                    rt.line_found_cnt   = 0;
                    rt.sub_brake_until  = {};
                    rt.target_right_distance = Config::RIGHT_MAX_CM;
                    rt.forward_min_until = {};
                    rt.right_counter = 0;
                    rt.ob_seek_force_full_duration = false;
                    rt.ob_left_break_active = false;
                    rt.ob_left_break_until  = {};
                    leds_for_ob_phase(rt.phase);
                }

                // Micro-brake so motors don't go boom
                if (Kernel::Clock::now() < rt.sub_brake_until) {
                    motors_brake(Config::BRAKE_STRENGTH);
                    break;
                }

                //state transition helper
                auto set_phase = [&](ObPhase p) {
                    auto now = Kernel::Clock::now();

                    if (rt.phase != p) {
                        rt.phase = p;
                        rt.sub_brake_until = now + 50ms;
                        leds_for_ob_phase(rt.phase);
                    }
                };

                // Line exit condition -> debounced tho
                if (rt.did_follow && li.middle) rt.line_found_cnt++;

                if (rt.line_found_cnt >= 2) {
                    rt.ob_active = false;
                    rt.found_cnt = 0;

                    // Run a short left brake-inner turn after reacquiring the line
                    rt.ob_left_break_active = true;
                    rt.ob_left_break_until  = Kernel::Clock::now() + 350ms;

                    // Clear any old seek lock state
                    rt.seek_lock_active = false;
                    rt.seek_lock_dir    = 0;
                    rt.ob_seek_force_full_duration = false;

                    enter_state(CtrlState::SEEK_LEFT, 0);
                    break;
                }

                // Rudementary obstacle check
                const bool right_seen = sh.right_ok && sh.right_cm > 0.0f && sh.right_cm < Config::OB_RIGHT_TRIG_CM;

                if(right_seen && sh.right_cm < rt.target_right_distance) {
                    rt.target_right_distance = sh.right_cm;
                }

                if (!right_seen || sh.right_cm > rt.target_right_distance) {
                    rt.right_counter++;
                }

                //sub-state machine
                switch (rt.phase) {
                    case ObPhase::MOVE_CLOSER: {
                        move_forward(Config::OB_FRONT_CLOSER_SPEED);

                        if(sh.front_cm <= Config::OB_FRONT_CLOSE_CM || !sh.front_ok) { // if we got close enough or lost the reading (maybe we went past the object), start turning left to find a way around
                            set_phase(ObPhase::TURN_LEFT_45);
                        }
                        break;
                    }
                    case ObPhase::TURN_LEFT_45: {
                        if (rt.phase_until.time_since_epoch().count() == 0) {
                            rt.phase_until = Kernel::Clock::now() + std::chrono::milliseconds(Config::TURN_45_MS);
                        }

                        turn_left_skid_reverse_inner(Config::DUTY_OB_TURN);

                        if (Kernel::Clock::now() >= rt.phase_until) {
                            rt.phase_until = {};
                            rt.right_counter = 0;
                            set_phase(ObPhase::SEARCH_RIGHT_WALL);
                        }
                        break;
                    }

                    //move forward until obstacle starts moving further away -> then turn right
                    case ObPhase::SEARCH_RIGHT_WALL: {
                        if (rt.forward_min_until.time_since_epoch().count() == 0) {
                            rt.forward_min_until = Kernel::Clock::now() + std::chrono::milliseconds(Config::OB_PHASE_MIN_MS);
                        }

                        move_forward(Config::DUTY_OB_FWD);

                        if (right_seen && !rt.reacq_has_seen) {
                            rt.reacq_has_seen = true;
                            rt.right_counter = 0;

                        }

                        if(rt.reacq_has_seen) {
                        }
                        else if (Kernel::Clock::now() <= rt.forward_min_until) {
                            rt.right_counter = 0; // reset counter during forward phase until min time is hit at least once, then allow it to trigger reacquisition phase if obstacle is moving away
                            break;
                        }

                        // only allow leaving forward phase after min time elapsed
                        if (rt.right_counter >= 1) {
                            rt.right_counter = 0;
                            rt.forward_min_until = {};     
                            rt.reacq_has_seen = false;
                            rt.target_right_distance = sh.right_cm; // reset target distance for reacquisition phase
                            set_phase(ObPhase::REACQUIRE);
                            rt.did_follow = true;                            
                        }

                        break;
                    }

                    //turn right until obstacle is moving away again -> then go forward and keep looking for line, rinse and repeat until line is found
                    case ObPhase::REACQUIRE: {
                        // keep forward dwell re-arm clean
                        rt.forward_min_until = {};

                        // Stage 1: keep turning until we see the obstacle at least once
                        if (!rt.reacq_has_seen) {
                            turn_right_skid_reverse_inner(Config::DUTY_OB_TURN);

                            if (right_seen) {
                                rt.reacq_has_seen = true;
                                rt.right_counter = 0;
                            }
                            break;
                        }

                        turn_right_skid_reverse_inner(Config::DUTY_OB_TURN);

                        if (rt.reacq_has_seen && !right_seen) { // if it's moving away, try to reacquire line/ {rt.right_counter >= 10}
                            rt.right_counter = 0;
                            rt.reacq_has_seen = false;
                            set_phase(ObPhase::SEARCH_RIGHT_WALL);
                        }

                        break;
                    }
                }

                break;
            }

            case CtrlState::STOPPED: {
                // Brake on entry for a short window
                if (Kernel::Clock::now() < stop_brake_until) {
                    motors_brake(Config::BRAKE_STRENGTH);
                    break;
                }

                const bool red_now   = (ls == LightState::RED);
                const bool green_now = (ls == LightState::GREEN);

                // Debounce green -> leave STOPPED
                if (green_now) rt.stop_green_cnt++;
                else           rt.stop_green_cnt = 0;

                if (rt.stop_green_cnt >= 2) {
                    rt.stop_green_cnt = 0;
                    rt.stop_no_red_cnt = 0;
                    rt.stop_red_reacq_cnt = 0;
                    rt.stop_phase = ControllerRuntime::StopPhase::HOLD_ON_RED;

                    found_light_again = false; // reset reacquisition flag for next time we stop

                    enter_state(CtrlState::FOLLOW, 0);
                    break;
                }

                // Otherwise, handle stop sub-state
                if (rt.stop_phase == ControllerRuntime::StopPhase::HOLD_ON_RED) {
                    motors_all_off();

                    if (red_now) {
                        rt.stop_no_red_cnt = 0;
                    } else {
                        rt.stop_no_red_cnt++;
                        if (rt.stop_no_red_cnt >= Config::LOST_RED_TICKS_BEFORE_REV && !found_light_again) {
                            rt.stop_no_red_cnt = 0;
                            rt.stop_red_reacq_cnt = 0;
                            rt.stop_phase = ControllerRuntime::StopPhase::BACKUP_FIND_RED;
                        }
                    }
                } else { // BACKUP_FIND_RED
                    move_backward(Config::DUTY_REV_FIND_RED);

                    if (red_now) rt.stop_red_reacq_cnt++;
                    else         rt.stop_red_reacq_cnt = 0;

                    if (rt.stop_red_reacq_cnt >= Config::RED_REACQUIRE_TICKS) {
                        rt.stop_red_reacq_cnt = 0;
                        found_light_again = true;
                        rt.stop_phase = ControllerRuntime::StopPhase::HOLD_ON_RED;
                        motors_all_off();
                    }
                }

                break;
            }

            case CtrlState::FULLY_STOPPED: {
                if (!state_time_elapsed()) motors_brake(Config::FULL_STOP_BRAKE_STRENGTH);
                else                       motors_all_off();
                break;
            }

            case CtrlState::MANUAL_BT:
            default:
                // Manual is driven by BT handler in main
                break;
        }
    }

    //======================================================
    //=================== MAIN TASKS =======================
    //======================================================

    static void task_ultrasonic(UltrasonicSensor& us_front, UltrasonicSensor& us_right) {
        static bool read_front_next = true;

        if (read_front_next) {
            auto rf = us_front.read_cm(Config::FRONT_MAX_CM);

            core_util_critical_section_enter();
            g_shared.front_state = rf.state;
            g_shared.front_stamp = Kernel::Clock::now();
            g_shared.front_ok    = rf.valid;
            if (rf.valid) g_shared.front_cm = rf.cm;     // keep previous cm if invalid
            core_util_critical_section_exit();
        } else {
            auto rr = us_right.read_cm(Config::RIGHT_MAX_CM);

            core_util_critical_section_enter();
            g_shared.right_state = rr.state;
            g_shared.right_stamp = Kernel::Clock::now();
            g_shared.right_ok    = rr.valid;
            if (rr.valid) g_shared.right_cm = rr.cm;     // keep previous cm if invalid
            core_util_critical_section_exit();
        }
        read_front_next = !read_front_next;

        // Debug snapshot: still shows latest values for both (one updated this tick)
        const Shared sh = shared_snapshot();
        Debug::update_ultra(Debug::make_ultra(
            sh.front_cm, sh.front_ok, sh.front_state,
            sh.right_cm, sh.right_ok, sh.right_state
        ));
    }

    static void task_color(bool tcs_ok) {
        if (!tcs_ok) return;

        auto v = HW::color.read_raw(true, 1);

        static LightState cand     = LightState::NONE;
        static int        cand_cnt = 0;

        if (v.valid) {
            const LightState s = classify_light(v);
            if (s == cand) cand_cnt++;
            else { cand = s; cand_cnt = 1; }

            shared_set_color(v, true, cand);
        } else {
            // Keep last rgbc value, just mark invalid
            shared_set_color(g_shared.rgbc, false, g_shared.light);
        }

        const Shared sh = shared_snapshot();
        Debug::update_color(Debug::make_color(sh.rgbc, sh.rgbc_valid, sh.light));
    }

 static void task_bluetooth() {
    if (!bt_connected()) return;
    if (!Debug::bt_read_char(bt_char)) return;

    char c = bt_char;              // existing logic uses this

    if (c == '\r' || c == '\n' || c < 32 || c > 126) return;

    if (c == 'w') {
        manual_mode = true;
        cancel_obstacle_mode();
        enter_state(CtrlState::MANUAL_BT, 0);
        leds_for_manual_cmd('W');             
        move_forward(Config::BT_DUTY_FWD_SLOW); 
        return;
    }
    if (c == 's') {
        manual_mode = true;
        cancel_obstacle_mode();
        enter_state(CtrlState::MANUAL_BT, 0);
        leds_for_manual_cmd('S');
        move_backward(Config::BT_DUTY_REV_SLOW);
        return;
    }
    if (c == 'a') {
        manual_mode = true;
        cancel_obstacle_mode();
        enter_state(CtrlState::MANUAL_BT, 0);
        leds_for_manual_cmd('A');
        turn_left_break_inner(0.9, 0.82); 
    }
    if (c == 'd') {
        manual_mode = true;
        cancel_obstacle_mode();
        enter_state(CtrlState::MANUAL_BT, 0);
        leds_for_manual_cmd('D');
        turn_right_break_inner(0.9, 0.82); 
        return;
    }
    if (c == 'z' || c == 'Z') {
        manual_mode = true;
        cancel_obstacle_mode();
        enter_state(CtrlState::MANUAL_BT, 0);
        leds_for_manual_cmd('Z');
        motors_coast();
        return;
    }

    // ---- EXISTING BEHAVIOR (unchanged) ----
    if (c == 'W' || c == 'A' || c == 'S' || c == 'D') {
        manual_mode = true;
        cancel_obstacle_mode();
        enter_state(CtrlState::MANUAL_BT, 0);
        leds_for_manual_cmd(c);

        if (c == 'W') move_forward(Config::BT_DUTY_FWD);                
        if (c == 'S') move_backward(Config::BT_DUTY_REV);               
        if (c == 'A') turn_left_skid_reverse_inner(Config::BT_DUTY_TURN);  
        if (c == 'D') turn_right_skid_reverse_inner(Config::BT_DUTY_TURN); 
        return;
    }

    if (c == 'Q' || c == 'q') {
        manual_mode = true;
        cancel_obstacle_mode();
        enter_state(CtrlState::FULLY_STOPPED, Config::FULL_STOP_BRAKE_MS);
        motors_brake(Config::FULL_STOP_BRAKE_STRENGTH);
        Debug::log("BT: FULLY_STOPPED\r\n");
        return;
    }

    if (c == 'X' || c == 'x') {
        manual_mode = false;
        cancel_obstacle_mode();
        enter_state((last_pos >= 0) ? CtrlState::SEEK_RIGHT : CtrlState::SEEK_LEFT, 0);
        Debug::log("BT: EXIT MANUAL -> SEEK\r\n");
        return;
    }

    Debug::log("BT: unknown cmd\r\n");
}
    //======================================================
    //========================== MAIN ======================
    //======================================================
    
    int main() {

        Debug::init(true, true, HW::BT_TX, HW::BT_RX, HW::BT_BAUD);
        manual_mode = false;

        motors_init(Config::PWM_FREQ_HZ);

        //done here so that the trig pin gets set to a known state
        UltrasonicSensor us_front(D8,  D9);
        UltrasonicSensor us_right(D10, D11);

        const bool tcs_ok = HW::color.init(100.0f, tcs3472::Gain::X16);
        if (!tcs_ok) Debug::log("TCS3472 init failed");

        // START BEHAVIOR!!!! MAKE FULL STOPPED FOR BLUETOOTH -> SEEK FOR QUICK TESTS
        enter_state(CtrlState::SEEK_LEFT, 0);

        Flags::colorTick.attach(&Flags::color_isr,   std::chrono::milliseconds(Config::COLOR_PERIOD_MS));
        Flags::controlTick.attach(&Flags::control_isr, std::chrono::milliseconds(Config::CTRL_PERIOD_MS));
        Flags::ultraTick.attach(&Flags::ultra_isr,   std::chrono::milliseconds(Config::ULTRA_PERIOD_MS));
        Flags::debugTick.attach(&Flags::debug_isr,   std::chrono::milliseconds(Config::DEBUG_PERIOD_MS));

        while (true) {
            bool do_update = false;
            bool do_ultra  = false;
            bool do_color  = false;
            bool do_debug  = false;

            core_util_critical_section_enter();
            if (Flags::control_due) { Flags::control_due = false; do_update = true; }
            if (Flags::ultra_due)   { Flags::ultra_due   = false; do_ultra  = true; }
            if (Flags::color_due)   { Flags::color_due   = false; do_color  = true; }
            if (Flags::debug_due)   { Flags::debug_due   = false; do_debug  = true; }
            core_util_critical_section_exit();

            if (do_update) controller_update();
            if (do_ultra)  task_ultrasonic(us_front, us_right);
            if (do_color)  task_color(tcs_ok);
            if (do_debug)  Debug::tick();

            task_bluetooth();
        }

        return 0;
        
    }
    