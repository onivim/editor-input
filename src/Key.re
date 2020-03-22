type t = 
| Character(char)
| Function(int)
| NumpadDigit(int)
| Escape
| Down
| Up
| Left
| Right
| Tab
| PageUp
| PageDown
| Return
| Space
| Delete
| Pause
| Home
| End
| Backspace
| CapsLock
| Insert;

let to_string = fun
| Character(c) => Printf.sprintf("Character(%c)", c)
| Function(digit) => Printf.sprintf("Function(%d)", digit)
| NumpadDigit(digit) => Printf.sprintf("Numpad(%d)", digit)
| Escape => "Escape"
| Down => "Down"
| Up => "Up"
| Left => "Left"
| Right => "Right"
| Tab => "Tab"
| PageUp => "PageUp"
| PageDown => "PageDown"
| Return => "Return"
| Space => "Space"
| Delete => "Delete"
| Pause => "PauseBreak"
| Home => "Home"
| End => "End"
| Backspace => "Backspace"
| CapsLock => "CapsLock"
| Insert => "Insert"
