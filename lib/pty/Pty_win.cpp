#include "Pty.h"

// Windows stub implementation when PTY backend disabled or using future ConPTY.
// Currently provides a minimal object so higher level code can link.
#ifdef _WIN32
namespace Konsole {
// Nothing extra needed; functionality will be implemented when real Windows backend lands.
}
#endif
