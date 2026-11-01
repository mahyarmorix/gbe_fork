// Jong (the Jong launcher) overlay link: Shift+Tab is sent to Jong over the named pipe \\.\pipe\JongOverlay
// instead of drawing this emulator's own overlay, and Jong answers with its overlay's state and whom to invite / join.
// One JSON object per line. Nothing happens (and nothing fails) when Jong isn't running.
#ifndef __INCLUDED_JONG_PIPE_H__
#define __INCLUDED_JONG_PIPE_H__

#include <string>

namespace jong_pipe {
    // starts the background connection (once); retried every few seconds while Jong isn't there
    void start();
    // writes one line (a '\n' is added); false when not connected
    bool send(const std::string &line);
    // the next line Jong sent, if any
    bool poll(std::string &line);
}

#endif // __INCLUDED_JONG_PIPE_H__
