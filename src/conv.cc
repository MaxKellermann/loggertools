#include "tp.hh"

int main(int argc, char **argv) {
    TurnPointReader *reader = new SeeYouTurnPointReader("/home/max/dl/tp/alps.cup");
    TurnPointWriter *writer = new SeeYouTurnPointWriter("/proc/self/fd/1");
    const TurnPoint *tp;

    (void)argc;
    (void)argv;

    while ((tp = reader->read()) != NULL) {
        writer->write(*tp);
        delete tp;
    }

    return 0;
}
