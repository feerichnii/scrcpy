#ifndef SC_GUI_ISCRCPY_SESSION_H
#define SC_GUI_ISCRCPY_SESSION_H

#include "gui/models.h"

#include <string>

namespace scgui {

class IScrcpySession {
public:
    virtual ~IScrcpySession() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual SessionState state() const = 0;
    virtual const std::string &serial() const = 0;
};

} // namespace scgui

#endif
