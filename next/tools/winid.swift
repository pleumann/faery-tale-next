// Print the CGWindowID of the first on-screen window owned by the given process id.
// Usage: swift tools/winid.swift <pid>
import CoreGraphics
import Foundation

guard CommandLine.arguments.count > 1, let pid = Int(CommandLine.arguments[1]) else { exit(1) }
let list = CGWindowListCopyWindowInfo([.optionAll], kCGNullWindowID) as? [[String: Any]] ?? []
for w in list {
    if let owner = w[kCGWindowOwnerPID as String] as? Int, owner == pid,
       let layer = w[kCGWindowLayer as String] as? Int, layer == 0,
       let name = w[kCGWindowName as String] as? String, name.contains("CSpect"),
       let id = w[kCGWindowNumber as String] as? Int {
        print(id)
        exit(0)
    }
}
exit(1)
