#include "./Mouse.h"
#include "./Utils.h"

#ifdef _WIN32
#include <Windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <iostream>
#include <cstring>
#endif

namespace Robot {

unsigned int Mouse::delay = 16;
bool Mouse::isPressed = false;
MouseButton Mouse::pressedButton = MouseButton::LEFT_BUTTON;

#ifdef __linux__
  Display* Mouse::display = nullptr;
  Window Mouse::rootWindow = 0;

  void Mouse::InitializeX11() {
    if (display == nullptr) {
      display = XOpenDisplay(nullptr);
      if (display == nullptr) {
        throw std::runtime_error("Cannot open X11 display");
      }
      rootWindow = DefaultRootWindow(display);
    }
  }

  void Mouse::CleanupX11() {
    if (display != nullptr) {
      XCloseDisplay(display);
      display = nullptr;
    }
  }
#endif

#ifdef _WIN32
POINT Mouse::getCurrentPosition() {
  POINT winPoint;
  GetCursorPos(&winPoint);
  return winPoint;
}
#elif __APPLE__
CGPoint Mouse::getCurrentPosition() {
  CGEventRef event = CGEventCreate(nullptr);
  CGPoint cursor = CGEventGetLocation(event);
  CFRelease(event);
  return cursor;
}
#elif __linux__
  Robot::Point Mouse::getCurrentPosition() {
    InitializeX11();

    Robot::Point point;
    Window root_return, child_return;
    int root_x, root_y;
    int win_x, win_y;
    unsigned int mask_return;

    XQueryPointer(display, rootWindow, &root_return, &child_return,
                  &root_x, &root_y, &win_x, &win_y, &mask_return);

    point.x = root_x;
    point.y = root_y;

    return point;
  }
#endif

void Mouse::Move(Robot::Point point) {
#ifdef _WIN32
  SetCursorPos(point.x, point.y);
#elif __APPLE__
  CGPoint target = CGPointMake(point.x, point.y);
  CGEventRef event = CGEventCreateMouseEvent(
      nullptr,
      kCGEventMouseMoved,
      target,
      kCGMouseButtonLeft
  );
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);

  if (Mouse::isPressed) {
    Mouse::MoveWithButtonPressed(point, Mouse::pressedButton);
  }
#elif __linux__
  InitializeX11();

  // Move the mouse using XTest
  XTestFakeMotionEvent(display, -1, point.x, point.y, CurrentTime);
  XFlush(display);

  if (Mouse::isPressed) {
    Mouse::MoveWithButtonPressed(point, Mouse::pressedButton);
  }
#endif
}

Robot::Point Mouse::GetPosition() {
  // TODO: how long exactly should we wait?
  Robot::delay(16);

  Robot::Point point;
#ifdef _WIN32
  POINT cursor = getCurrentPosition();
#elif __APPLE__
  CGPoint cursor = getCurrentPosition();
#elif __linux__
    Robot::Point cursor = getCurrentPosition();
    point.x = cursor.x;
    point.y = cursor.y;
#endif
  point.x = cursor.x;
  point.y = cursor.y;

  return point;
}

void Mouse::ToggleButton(bool down, MouseButton button, bool doubleClick) {
#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_MOUSE;
  input.mi.dwFlags =
      (button == MouseButton::LEFT_BUTTON
           ? (down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP)
       : button == MouseButton::RIGHT_BUTTON
           ? (down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP)
           : (down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP));
  SendInput(1, &input, sizeof(INPUT));
#elif __APPLE__
  CGPoint currentPosition = getCurrentPosition();

  CGEventType buttonType;
  switch (button) {
    case MouseButton::LEFT_BUTTON:
      buttonType = down ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;
      break;
    case MouseButton::RIGHT_BUTTON:
      buttonType = down ? kCGEventRightMouseDown : kCGEventRightMouseUp;
      break;
    case MouseButton::CENTER_BUTTON:
      buttonType = down ? kCGEventOtherMouseDown : kCGEventOtherMouseUp;
      break;
  }

  CGEventRef buttonEvent = CGEventCreateMouseEvent(
      nullptr,
      buttonType,
      currentPosition,
      (button == MouseButton::CENTER_BUTTON) ? kCGMouseButtonCenter
                                             : kCGMouseButtonLeft
  );

  if (doubleClick) {
    CGEventSetIntegerValueField(buttonEvent, kCGMouseEventClickState, 2);
  }

  CGEventPost(kCGHIDEventTap, buttonEvent);
  CFRelease(buttonEvent);
#elif __linux__
    InitializeX11();

    unsigned int buttonCode;
    switch (button) {
      case MouseButton::LEFT_BUTTON:
        buttonCode = 1;
        break;
      case MouseButton::RIGHT_BUTTON:
        buttonCode = 3;
        break;
      case MouseButton::CENTER_BUTTON:
        buttonCode = 2;
        break;
      default:
        buttonCode = 1;
    }

    if (down) {
      XTestFakeButtonEvent(display, buttonCode, True, CurrentTime);
    } else {
      XTestFakeButtonEvent(display, buttonCode, False, CurrentTime);
    }

    XFlush(display);

    // Handle double click
    if (doubleClick && !down) {
      Robot::delay(10);
      XTestFakeButtonEvent(display, buttonCode, True, CurrentTime);
      XTestFakeButtonEvent(display, buttonCode, False, CurrentTime);
      XFlush(display);
    }
#endif

  if (down) {
    Mouse::isPressed = true;
    Mouse::pressedButton = button;
  } else {
    Mouse::isPressed = false;
  }
}

void Mouse::MoveWithButtonPressed(Robot::Point point, MouseButton button) {
#ifdef _WIN32
  // On Windows, just calling Move is enough as it will keep the button state.
  Mouse::Move(point);
#elif __APPLE__
  CGPoint target = CGPointMake(point.x, point.y);

  CGEventType dragEventType;
  CGMouseButton cgButton;
  switch (button) {
    case MouseButton::LEFT_BUTTON:
      dragEventType = kCGEventLeftMouseDragged;
      cgButton = kCGMouseButtonLeft;
      break;
    case MouseButton::RIGHT_BUTTON:
      dragEventType = kCGEventRightMouseDragged;
      cgButton = kCGMouseButtonRight;
      break;
    case MouseButton::CENTER_BUTTON:
      dragEventType = kCGEventOtherMouseDragged;
      cgButton = kCGMouseButtonCenter;
      break;
  }

  CGEventRef mouseDragEvent =
      CGEventCreateMouseEvent(nullptr, dragEventType, target, cgButton);
  CGEventPost(kCGHIDEventTap, mouseDragEvent);
  CFRelease(mouseDragEvent);
#elif __linux__
    InitializeX11();

    // For dragging, we simulate mouse motion with button pressed
    unsigned int buttonState = 0;
    switch (button) {
      case MouseButton::LEFT_BUTTON:
        buttonState = Button1Mask;
        break;
      case MouseButton::RIGHT_BUTTON:
        buttonState = Button3Mask;
        break;
      case MouseButton::CENTER_BUTTON:
        buttonState = Button2Mask;
        break;
    }

    // Create a motion event with the appropriate button state
    XTestFakeMotionEvent(display, -1, point.x, point.y, CurrentTime);

    // Ensure the button is held down
    unsigned int buttonCode;
    switch (button) {
      case MouseButton::LEFT_BUTTON:
        buttonCode = 1;
        break;
      case MouseButton::RIGHT_BUTTON:
        buttonCode = 3;
        break;
      case MouseButton::CENTER_BUTTON:
        buttonCode = 2;
        break;
    }

    XFlush(display);
#endif
}

void Mouse::Click(MouseButton button) {
  ToggleButton(true, button);
  Robot::delay(10);   // add a little delay
  ToggleButton(false, button);
}

void Mouse::DoubleClick(MouseButton button) {
  Click(button);
  Robot::delay(80);
  Click(button);
}

void Mouse::ScrollBy(int y, int x) {
#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_MOUSE;

  input.mi.dwFlags = MOUSEEVENTF_WHEEL;
  input.mi.mouseData = static_cast<DWORD>(WHEEL_DELTA * y);
  SendInput(1, &input, sizeof(INPUT));

  input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
  input.mi.mouseData = static_cast<DWORD>(WHEEL_DELTA * x);
  SendInput(1, &input, sizeof(INPUT));
#elif __APPLE__
  CGEventRef scrollEvent =
      CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 2, y, x);
  CGEventPost(kCGHIDEventTap, scrollEvent);
  CFRelease(scrollEvent);
#elif __linux__
    InitializeX11();

    // Vertical scrolling
    if (y != 0) {
      unsigned int buttonCode = (y > 0) ? 4 : 5;  // 4=up, 5=down
      for (int i = 0; i < std::abs(y); i++) {
        XTestFakeButtonEvent(display, buttonCode, True, CurrentTime);
        XTestFakeButtonEvent(display, buttonCode, False, CurrentTime);
        XFlush(display);
        Robot::delay(10);
      }
    }

    // Horizontal scrolling
    if (x != 0) {
      unsigned int buttonCode = (x > 0) ? 7 : 6;  // 6=left, 7=right
      for (int i = 0; i < std::abs(x); i++) {
        XTestFakeButtonEvent(display, buttonCode, True, CurrentTime);
        XTestFakeButtonEvent(display, buttonCode, False, CurrentTime);
        XFlush(display);
        Robot::delay(10);
      }
    }
#endif
}

void Mouse::Drag(Robot::Point toPoint) {
  Robot::Mouse::ToggleButton(true, Robot::MouseButton::LEFT_BUTTON);
  Robot::delay(10);
  Mouse::Move(toPoint);
  Robot::delay(10);
  Mouse::ToggleButton(false, MouseButton::LEFT_BUTTON);
}

void Mouse::MoveSmooth(Robot::Point point) {
  Robot::Point currentPosition = GetPosition();

  int dx = point.x - currentPosition.x;
  int dy = point.y - currentPosition.y;

  int steps = (std::abs(dx) > std::abs(dy)) ? std::abs(dx) : std::abs(dy);

  float deltaX = static_cast<float>(dx) / steps;
  float deltaY = static_cast<float>(dy) / steps;

  for (int i = 1; i <= steps; i++) {
    Robot::Point stepPosition;
    stepPosition.x = currentPosition.x + static_cast<int>(deltaX * i);
    stepPosition.y = currentPosition.y + static_cast<int>(deltaY * i);

    if (Mouse::isPressed) {
      MoveWithButtonPressed(stepPosition, Mouse::pressedButton);
    } else {
      Move(stepPosition);
    }

    Robot::delay(1);
  }
}

void Mouse::DragSmooth(Robot::Point toPoint) {
  Robot::Mouse::ToggleButton(true, Robot::MouseButton::LEFT_BUTTON);
  Robot::delay(10);
  Mouse::MoveSmooth(toPoint);
  Robot::delay(10);
  Mouse::ToggleButton(false, MouseButton::LEFT_BUTTON);
}

}  // namespace Robot
