#include "exerciser.h"

void exercise(connection *C) {

    // valid colors
    query2(C,"DarkBlue");
    query2(C,"LightBlue");
    // invalid color
    query2(C,"Maroon");
}
