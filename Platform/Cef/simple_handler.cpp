// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

// Modified by Chen Wenli @ 2018/12/21 for integration purpose

#include "simple_handler.hpp"

#include <sstream>
#include <string>

#include "base/cef_bind.h"
#include "cef_app.h"
#include "views/cef_browser_view.h"
#include "views/cef_window.h"
#include "wrapper/cef_closure_task.h"
#include "wrapper/cef_helpers.h"

#include "GfxConfiguration.hpp"
#include "IApplication.hpp"

using namespace My;

namespace My {
    static SimpleHandler* g_pInstance = NULL;
}

SimpleHandler::SimpleHandler(bool use_views)
    : use_views_(use_views), is_closing_(false) 
{
    DCHECK(!g_pInstance);
    g_pInstance = this;
}

SimpleHandler::~SimpleHandler() 
{
    g_pInstance = NULL;
}

// static
SimpleHandler* SimpleHandler::GetInstance() 
{
    return g_pInstance;
}

void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                const CefString& title) 
{
    CEF_REQUIRE_UI_THREAD();

    std::string prefix (g_pApp->GetConfiguration().appName);

    if (use_views_) {
        // Set the title of the window using the Views framework.
        CefRefPtr<CefBrowserView> browser_view =
            CefBrowserView::GetForBrowser(browser);
        if (browser_view) {
            CefRefPtr<CefWindow> window = browser_view->GetWindow();
            if (window)
                window->SetTitle(prefix + " [" + title.ToString() + "]");
        }
    } else {
        // Set the title of the window using platform APIs.
        PlatformTitleChange(browser, prefix + " [" + title.ToString() + "]");
    }
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) 
{
    CEF_REQUIRE_UI_THREAD();

    // Add to the list of existing browsers.
    browser_list_.push_back(browser);
}

bool SimpleHandler::DoClose(CefRefPtr<CefBrowser> browser) 
{
    CEF_REQUIRE_UI_THREAD();

    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed destription of this
    // process.
    if (browser_list_.size() == 1) {
        // Set a flag to indicate that the window close should be allowed.
        is_closing_ = true;
    }

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) 
{
    CEF_REQUIRE_UI_THREAD();

    // Remove from the list of existing browsers.
    BrowserList::iterator bit = browser_list_.begin();
    for (; bit != browser_list_.end(); ++bit) {
        if ((*bit)->IsSame(browser)) {
        browser_list_.erase(bit);
        break;
        }
    }

    if (browser_list_.empty()) {
        // All browser windows have closed. Quit the application message loop.
        g_pApp->RequestQuit();
    }
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) 
{
    CEF_REQUIRE_UI_THREAD();

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    // Display a load error message.
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
            "<h2>Failed to load URL "
        << std::string(failedUrl) << " with error " << std::string(errorText)
        << " (" << errorCode << ").</h2></body></html>";
    frame->LoadString(ss.str(), failedUrl);
}

void SimpleHandler::CloseAllBrowsers(bool force_close) 
{
    if (!CefCurrentlyOn(TID_UI)) {
        // Execute on the UI thread.
        CefPostTask(TID_UI, base::Bind(&SimpleHandler::CloseAllBrowsers, this,
                                    force_close));
        return;
    }

    if (browser_list_.empty())
        return;

    BrowserList::const_iterator it = browser_list_.begin();
    for (; it != browser_list_.end(); ++it)
    {
        (*it)->GetHost()->CloseBrowser(force_close);
    }
}
