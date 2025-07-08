//
//  Interaction.swift
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

import AppKit

public protocol Interactive {
    func onMouseMove(_ offset: Position3d)
    func onKeyPress(_ key: Key)
    func eachFrame(_ self: inout Interactive)
}

public extension Interactive {
    func onMouseMove(_ offset: Position3d) {}
    func onKeyPress(_ key: Key) {}
    func eachFrame(_ self: inout Interactive) {}
}

public enum Key: UInt16, CaseIterable, Sendable {
    // Letters
    case a = 0
    case s = 1
    case d = 2
    case f = 3
    case h = 4
    case g = 5
    case z = 6
    case x = 7
    case c = 8
    case v = 9
    case b = 11
    case q = 12
    case w = 13
    case e = 14
    case r = 15
    case y = 16
    case t = 17
    case one = 18
    case two = 19
    case three = 20
    case four = 21
    case six = 22
    case five = 23
    case equal = 24
    case nine = 25
    case seven = 26
    case minus = 27
    case eight = 28
    case zero = 29
    case rightBracket = 30
    case o = 31
    case u = 32
    case leftBracket = 33
    case i = 34
    case p = 35
    case l = 37
    case j = 38
    case quote = 39
    case k = 40
    case semicolon = 41
    case backslash = 42
    case comma = 43
    case slash = 44
    case n = 45
    case m = 46
    case period = 47
    case tab = 48
    case space = 49
    case grave = 50
    case delete = 51
    case enter = 52
    case escape = 53

    // Function keys
    case f1 = 122
    case f2 = 120
    case f3 = 99
    case f4 = 118
    case f5 = 96
    case f6 = 97
    case f7 = 98
    case f8 = 100
    case f9 = 101
    case f10 = 109
    case f11 = 103
    case f12 = 111
    case f13 = 105
    case f14 = 107
    case f15 = 113
    case f16 = 106
    case f17 = 64
    case f18 = 79
    case f19 = 80
    case f20 = 90

    // Arrow keys
    case leftArrow = 123
    case rightArrow = 124
    case downArrow = 125
    case upArrow = 126

    // Modifier keys
    case command = 55
    case shift = 56
    case capsLock = 57
    case option = 58
    case control = 59
    case rightShift = 60
    case rightOption = 61
    case rightControl = 62
    case function = 63

    // Number pad
    case keypad0 = 82
    case keypad1 = 83
    case keypad2 = 84
    case keypad3 = 85
    case keypad4 = 86
    case keypad5 = 87
    case keypad6 = 88
    case keypad7 = 89
    case keypad8 = 91
    case keypad9 = 92
    case keypadDecimal = 65
    case keypadMultiply = 67
    case keypadPlus = 69
    case keypadClear = 71
    case keypadDivide = 75
    case keypadEnter = 76
    case keypadMinus = 78
    case keypadEquals = 81

    // Additional keys
    case home = 115
    case pageUp = 116
    case forwardDelete = 117
    case end = 119
    case pageDown = 121
    case help = 114
    case volumeUp = 72
    case volumeDown = 73
    case mute = 74
}

extension Key {
    private nonisolated(unsafe) static var _pressedKeys: Set<Key> = []
    private static let queue = DispatchQueue(label: "atlas.key.pressed", attributes: .concurrent)

    public static var pressedKeys: Set<Key> {
        return queue.sync { _pressedKeys }
    }

    static func addPressedKey(_ key: Key) {
        queue.async(flags: .barrier) {
            _pressedKeys.insert(key)
        }
    }

    static func removePressedKey(_ key: Key) {
        queue.async(flags: .barrier) {
            _pressedKeys.remove(key)
        }
    }

    static func clearAllPressedKeys() {
        queue.async(flags: .barrier) {
            _pressedKeys.removeAll()
        }
    }

    public static func isPressed(_ key: Key) -> Bool {
        return pressedKeys.contains(key)
    }
}

extension NSView {
    @objc var canBecomeKeyView: Bool {
        get { return true }
        set {}
    }
}
