#include "luat_base.h"

/*
// shore frame data, fill from file or camera
int8 tflm_frame[96*96];
int do_person_detection(signed char* in, signed char* out);

// call do_person_detection
void xxx() {
    int8 result[2];
    int ret = do_person_detection(tflm_frame, result);
    if (ret == 0) {
        LLOGD("no_person %d has_person %d", result[0], result[1]);
    }
    // return to lua ? or sys.publish ?
}
/*
