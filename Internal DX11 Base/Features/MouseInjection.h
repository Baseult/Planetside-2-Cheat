#pragma once

// Shared mouse injection structures for NoRecoil and Aimbot
// Undocumented Windows Syscall for mouse injection
enum InjectedInputMouseOptions {
    left_up = 4,
    left_down = 2,
    right_up = 8,
    right_down = 16
};

struct InjectedInputMouseInfo {
    int move_direction_x; // Relative movement X
    int move_direction_y; // Relative movement Y
    unsigned int mouse_data;
    InjectedInputMouseOptions mouse_options;
    unsigned int time_offset_in_miliseconds;
    void* extra_info;
};



