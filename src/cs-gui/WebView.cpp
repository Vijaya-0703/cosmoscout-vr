////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "WebView.hpp"

#include "internal/WebViewClient.hpp"

#include <include/cef_app.h>
#include <thread>

namespace cs::gui {

////////////////////////////////////////////////////////////////////////////////////////////////////

class DevToolsClient : public CefClient {
 public:
  IMPLEMENT_REFCOUNTING(DevToolsClient);
};

////////////////////////////////////////////////////////////////////////////////////////////////////

WebView::WebView(const std::string& url, int width, int height, bool allowLocalFileAccess)
    : mClient(new detail::WebViewClient()) {
  resize(width, height);

  CefWindowInfo info;
  info.width  = width;
  info.height = height;
  info.SetAsWindowless(0);

  CefBrowserSettings browserSettings;
  browserSettings.windowless_frame_rate = 60;
  browserSettings.web_security          = allowLocalFileAccess ? STATE_DISABLED : STATE_ENABLED;

  mBrowser =
      CefBrowserHost::CreateBrowserSync(info, mClient, url, browserSettings, nullptr, nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

WebView::~WebView() {
  auto host = mBrowser->GetHost();
  while (!host->TryCloseBrowser()) {
    CefDoMessageLoopWork();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // magically called by the code above
  // delete mClient;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::setDrawCallback(DrawCallback const& callback) {
  mClient->GetInternalRenderHandler()->SetDrawCallback(callback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::setCursorChangeCallback(CursorChangeCallback const& callback) {
  mClient->GetInternalRenderHandler()->SetCursorChangeCallback(callback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::setRequestKeyboardFocusCallback(RequestKeyboardFocusCallback const& callback) {
  mClient->GetInternalRenderHandler()->SetRequestKeyboardFocusCallback(callback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::resize(int width, int height) const {
  mClient->GetInternalRenderHandler()->Resize(width, height);

  if (mBrowser) {
    mBrowser->GetHost()->WasResized();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool WebView::getColor(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const {
  return mClient->GetInternalRenderHandler()->GetColor(x, y, r, g, b, a);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t WebView::getRed(int x, int y) const {
  uint8_t value(0), tmp(0);
  mClient->GetInternalRenderHandler()->GetColor(x, y, value, tmp, tmp, tmp);
  return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t WebView::getGreen(int x, int y) const {
  uint8_t value(0), tmp(0);
  mClient->GetInternalRenderHandler()->GetColor(x, y, tmp, value, tmp, tmp);
  return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t WebView::getBlue(int x, int y) const {
  uint8_t value(0), tmp(0);
  mClient->GetInternalRenderHandler()->GetColor(x, y, tmp, tmp, value, tmp);
  return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t WebView::getAlpha(int x, int y) const {
  uint8_t value(0), tmp(0);
  mClient->GetInternalRenderHandler()->GetColor(x, y, tmp, tmp, tmp, value);
  return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool WebView::getIsInteractive() const {
  return mInteractive;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::setIsInteractive(bool interactive) {
  mInteractive = interactive;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool WebView::getCanScroll() const {
  return mCanScroll;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::setCanScroll(bool canScroll) {
  mCanScroll = canScroll;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int WebView::getWidth() const {
  return mClient->GetInternalRenderHandler()->GetWidth();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int WebView::getHeight() const {
  return mClient->GetInternalRenderHandler()->GetHeight();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::waitForFinishedLoading() const {
  mClient->GetInternalLoadHandler()->WaitForFinishedLoading();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::reload(bool ignoreCache) const {
  if (ignoreCache)
    mBrowser->ReloadIgnoreCache();
  else
    mBrowser->Reload();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::goBack() const {
  mBrowser->GoBack();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::goForward() const {
  mBrowser->GoForward();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::cut() const {
  mBrowser->GetFocusedFrame()->Cut();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::copy() const {
  mBrowser->GetFocusedFrame()->Copy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::paste() const {
  mBrowser->GetFocusedFrame()->Paste();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::remove() const {
  mBrowser->GetFocusedFrame()->Delete();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::undo() const {
  mBrowser->GetFocusedFrame()->Undo();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::redo() const {
  mBrowser->GetFocusedFrame()->Redo();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::injectFocusEvent(bool focus) {
  if (mInteractive) {
    mBrowser->GetHost()->SendFocusEvent(focus);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::injectMouseEvent(MouseEvent const& event) {
  if (!mInteractive || (!mCanScroll && event.mType == MouseEvent::Type::eScroll)) {
    return;
  }

  CefMouseEvent cef_event;
  cef_event.modifiers = (uint32)mMouseModifiers;
  cef_event.x         = mMouseX;
  cef_event.y         = mMouseY;

  switch (event.mType) {
  case MouseEvent::Type::eMove:
    cef_event.x = mMouseX = event.mX;
    cef_event.y = mMouseY = event.mY;
    mBrowser->GetHost()->SendMouseMoveEvent(cef_event, false);
    break;

  case MouseEvent::Type::eLeave:
    mBrowser->GetHost()->SendMouseMoveEvent(cef_event, true);
    break;

  case MouseEvent::Type::eScroll:
    mBrowser->GetHost()->SendMouseWheelEvent(cef_event, event.mX, event.mY);
    break;

  case MouseEvent::Type::ePress:
    if (event.mButton == Button::eLeft) {
      mMouseModifiers |= int(Modifier::eLeftButton);
      double elpasedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - mLastClick)
                               .count();
      if (elpasedTime < 200) {
        mClickCount++;
      } else {
        mClickCount = 1;
      }
      mLastClick = std::chrono::steady_clock::now();
    } else if (event.mButton == Button::eRight) {
      mMouseModifiers |= int(Modifier::eRightButton);
    } else if (event.mButton == Button::eMiddle) {
      mMouseModifiers |= int(Modifier::eMiddleButton);
    }

    cef_event.modifiers = (uint32)mMouseModifiers;
    mBrowser->GetHost()->SendMouseClickEvent(
        cef_event, (cef_mouse_button_type_t)event.mButton, false, mClickCount);
    break;

  case MouseEvent::Type::eRelease:
    if (event.mButton == Button::eLeft) {
      mMouseModifiers &= ~int(Modifier::eLeftButton);
    } else if (event.mButton == Button::eRight) {
      mMouseModifiers &= ~int(Modifier::eRightButton);
    } else if (event.mButton == Button::eMiddle) {
      mMouseModifiers &= ~int(Modifier::eMiddleButton);
    }

    cef_event.modifiers = (uint32)mMouseModifiers;
    mBrowser->GetHost()->SendMouseClickEvent(
        cef_event, (cef_mouse_button_type_t)event.mButton, true, mClickCount);
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::injectKeyEvent(KeyEvent const& event) {
  if (!mInteractive || event.mType == KeyEvent::Type::eInvalid) {
    return;
  }

  CefKeyEvent cef_event;
  cef_event.modifiers               = event.mModifiers;
  cef_event.character               = event.mCharacter;
  cef_event.is_system_key           = false;
  cef_event.windows_key_code        = (int)event.mKey;
  cef_event.focus_on_editable_field = true;

  if (event.mType == KeyEvent::Type::ePress) {
    cef_event.type = KEYEVENT_KEYDOWN;
  } else if (event.mType == KeyEvent::Type::eRelease) {
    cef_event.type = KEYEVENT_KEYUP;
  } else {
    cef_event.type = KEYEVENT_CHAR;
  }

  mBrowser->GetHost()->SendKeyEvent(cef_event);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::callJavascriptImpl(
    std::string const& function, std::vector<std::string> const& args) const {
  std::string call(function + "( ");
  for (auto& s : args) {
    call += s + ",";
  }
  call.back() = ')';

  CefRefPtr<CefFrame> frame = mBrowser->GetMainFrame();
  frame->ExecuteJavaScript(call, frame->GetURL(), 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::executeJavascript(std::string const& code) const {
  CefRefPtr<CefFrame> frame = mBrowser->GetMainFrame();
  frame->ExecuteJavaScript(code, frame->GetURL(), 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::registerCallback(
    std::string const& name, std::string const& comment, std::function<void()> const& callback) {
  registerJSCallbackImpl(name, comment, {},
      [this, callback](std::vector<std::optional<JSType>> const& args) { callback(); });
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::unregisterCallback(std::string const& name) {
  mClient->UnregisterJSCallback(name);

  // Also remove the function property on the CosmoScout.callbacks property.
  std::string cmd = R"(
    if (typeof CosmoScout !== 'undefined') {
      delete CosmoScout.callbacks.$name;
    }
  )";

  utils::replaceString(cmd, "$name", name);
  executeJavascript(cmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WebView::registerJSCallbackImpl(std::string const& name, std::string const& comment,
    std::vector<std::type_index>&&                                   types,
    std::function<void(std::vector<std::optional<JSType>>&&)> const& callback) {

  // To increase the readability of the callback signature when inspected via an interactive
  // console, we name every argument depending on its type.
  std::string signature = "";

  const std::unordered_map<std::type_index, std::string> typeNames = {
      {std::type_index(typeid(double)), "double"}, {std::type_index(typeid(bool)), "bool"},
      {std::type_index(typeid(std::string)), "string"},
      {std::type_index(typeid(std::optional<double>)), "optionalDouble"},
      {std::type_index(typeid(std::optional<bool>)), "optionalBool"},
      {std::type_index(typeid(std::optional<std::string>)), "optionalString"}};

  std::unordered_map<std::type_index, int> typeCounts;

  for (int i(0); i < types.size(); ++i) {
    signature += typeNames.at(types[i]);

    if (typeCounts[types[i]]++ > 0) {
      signature += std::to_string(typeCounts[types[i]]);
    }

    if (i + 1 < types.size()) {
      signature += ", ";
    }
  }

  // When executing the 'window.callNative()' method, we need the callback's name as first
  // parameter.
  std::string callSignature = "'" + name + "'" + (signature == "" ? "" : ", " + signature);

  // Format the comment. This is a bit more involved since we do line wrapping for long comments.
  std::string formattedComment = "  // ";
  size_t      currentSpacePos  = 0;
  size_t      currentLineWidth = 0;
  while (currentSpacePos != std::string::npos) {
    size_t nextSpacePos = comment.find_first_of(' ', currentSpacePos + 1);
    formattedComment += comment.substr(currentSpacePos, nextSpacePos - currentSpacePos);
    currentLineWidth += nextSpacePos - currentSpacePos;
    currentSpacePos = nextSpacePos;

    if (currentSpacePos != std::string::npos && currentLineWidth > 40) {
      formattedComment += "\n  //";
      currentLineWidth = 0;
    }
  }

  // This registers the callback as a property of the CosmoScout.callbacks object. As the name may
  // contain multiple dots, this is a little tricky. We have to create multiple chained objects;
  // e.g. for the callback "notifications.print.warning", we first have to create the object
  // "notifications", then "print" and then the function "warning".
  std::string cmd = R"(
if (typeof CosmoScout !== 'undefined') {
let components = '$name'.split('.');
components.reduce((a, b) => a[b] = a[b] || {}, CosmoScout.callbacks);
CosmoScout.callbacks.$name = ($signature) => {

$comment

  window.callNative($callSignature);
}
})";

  utils::replaceString(cmd, "$name", name);
  utils::replaceString(cmd, "$comment", formattedComment);
  utils::replaceString(cmd, "$signature", signature);
  utils::replaceString(cmd, "$callSignature", callSignature);
  executeJavascript(cmd);

  // Register the actual 'window.callNative()' handler.
  mClient->RegisterJSCallback(name, callback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace cs::gui
