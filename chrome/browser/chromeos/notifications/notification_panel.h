// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Draws the view for the balloons.

#ifndef CHROME_BROWSER_CHROMEOS_NOTIFICATIONS_NOTIFICATION_PANEL_H_
#define CHROME_BROWSER_CHROMEOS_NOTIFICATIONS_NOTIFICATION_PANEL_H_

#include "base/task.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/chromeos/frame/panel_controller.h"
#include "chrome/browser/chromeos/notifications/balloon_collection_impl.h"
#include "chrome/common/notification_registrar.h"
#include "gfx/rect.h"

class Balloon;
class Notification;

namespace views {
class ScrollView;
}  // namespace views

namespace chromeos {

class BalloonContainer;
class BalloonViewImpl;
class NotificationPanelTester;

// NotificationPanel is a panel that displays notifications. It has
// several states and displays the different portion of notifications
// depending on in which state the panel is. The following shows
// how the panel's state changes in response to various events.
//
// TODO(oshima): add remove event and fix state transition graph below.
// Event List:
//   close: a user pressed close button on the title bar,
//          or the system closed the panel.
//   new : a new notification is added.
//   stale: one of new notifications became stale.
//   expand: a user pressed minimized panel to expand.
//   minimize: a user pressed the panel's title bar to minimize.
//   user: the user's mouse moved over the panel, indicates
//         that user is trying to interact with the panel.
// For state, see State enum's description below.
//
//
// [CLOSE]<-(event=close)-+     +--(event=stale, cond=has new|sticky)
//   |                    |     |         (event=new)
//   |                    |     V           |
//   +--(event=new)-->[STICKY_AND_NEW]----- +--------(event=user)
//   |                  ^           |                      |
//   |                  |  (event=stale,                   V
//   |                  |  cond=has new, no sticy)  +[ KEEP_SIZE ]<-+
//   |          (event=new)   (event=minimize)      |      |        |
//   |                  |           |               |      |        |
//   |                  |           |  (event=minimize)(event=close)|
//   |                  |           +---------------+      |        |
//   |                  |           V                      V        |
//   |                  [ MINIMIZED ]---(event=close)--> [CLOSE]    |
//   |                     |     ^                                  |
//   |                     |     |                                  |
//   |          (event=expand)  (event=minmize)                 (event=user)
//   |                     V     |                                  |
//   +--(event=open)---->[  FULL  ]-------------+-------------------+
//                         |     ^              |
//              (event=close)    +-------(event=stale)(event=new)
//                         |
//          [CLOSE] <------+
//
class NotificationPanel : public PanelController::Delegate,
                          public BalloonCollectionImpl::NotificationUI,
                          public NotificationObserver {
 public:
  enum State {
    FULL,  // Show all notifications
    KEEP_SIZE,  // Don't change the size.
    STICKY_AND_NEW,  // Show only new and sticky notifications.
    MINIMIZED,  // The panel is minimized.
    CLOSED,  // The panel is closed.
  };

  NotificationPanel();
  virtual ~NotificationPanel();

  // Shows/Hides the Panel.
  void Show();
  void Hide();

  // BalloonCollectionImpl::NotificationUI overrides..
  virtual void Add(Balloon* balloon);
  virtual bool Update(Balloon* balloon);
  virtual void Remove(Balloon* balloon);
  virtual void ResizeNotification(Balloon* balloon,
                                  const gfx::Size& size);
  virtual void SetActiveView(BalloonViewImpl* view);

  // PanelController overrides.
  virtual string16 GetPanelTitle();
  virtual SkBitmap GetPanelIcon();
  virtual void ClosePanel();

  // NotificationObserver overrides:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // Called when a mouse left the panel window.
  void OnMouseLeave();
  void OnMouseMotion(const gfx::Point& point);

  NotificationPanelTester* GetTester();

 private:
  friend class NotificationPanelTester;

  void Init();

  // Unregister the panel's state change notification.
  void UnregisterNotification();

  // Update the Panel Size according to its state.
  void UpdatePanel(bool update_panel_size);

  // Scroll the panel so that the |balloon| is visible.
  void ScrollBalloonToVisible(Balloon* balloon);

  // Update the notification's control view state.
  void UpdateControl();

  // Returns the panel's preferred bounds in the screen's coordinates.
  // The position will be controlled by window manager so
  // the origin is always (0, 0).
  gfx::Rect GetPreferredBounds();

  // Returns the bounds that covers sticky and new notifications.
  gfx::Rect GetStickyNewBounds();

  void StartStaleTimer(Balloon* balloon);

  // A callback function that is called when the notification
  // (that the view is associated with) becomes stale after a timeout.
  void OnStale(BalloonViewImpl* view);

  // Set the state. It can also print the
  void SetState(State, const char* method_name);

  // Mark the given notification as stale.
  void MarkStale(const Notification& notification);

  // True if the panel is visible.
  bool is_visible() {
    return state_ != CLOSED && state_ != MINIMIZED;
  }

  // Contains all notifications.
  BalloonContainer* balloon_container_;

  // The notification panel's widget.
  scoped_ptr<views::Widget> panel_widget_;

  // Panel controller for the notification panel.
  scoped_ptr<PanelController> panel_controller_;

  // A scrollable parent of the BalloonContainer.
  // This is owned by the panel so that we can re-attache to the widget
  // when closing and opening the panel.
  scoped_ptr<views::ScrollView> scroll_view_;

  // Panel's state.
  State state_;

  ScopedRunnableMethodFactory<NotificationPanel> task_factory_;

  // The minimum size of a notification.
  gfx::Rect min_bounds_;

  // Stale timeout.
  int stale_timeout_;

  // A registrar to subscribe PANEL_STATE_CHANGED event.
  NotificationRegistrar registrar_;

  // The notification a mouse pointer is currently on. NULL if the mouse
  // is out of the panel.
  BalloonViewImpl* active_;

  // A balloon that should be visible when it gets some size.
  Balloon* scroll_to_;

  // An object that provides interfacce for tests.
  scoped_ptr<NotificationPanelTester> tester_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPanel);
};

class NotificationPanelTester {
 public:
  explicit NotificationPanelTester(NotificationPanel* panel)
      : panel_(panel) {
  }

  NotificationPanel::State state() {
    return panel_->state_;
  }

  // Returns number of of sticky and new notifications.
  int GetNotificationCount() const;

  // Returns number of new notifications.
  int GetNewNotificationCount() const;

  // Returns number of of sticky notifications.
  int GetStickyNotificationCount() const;

  // Sets the timeout for a notification to become stale.
  void SetStaleTimeout(int timeout);

  // Mark the given notification as stale.
  void MarkStale(const Notification& notification);

  // Returns the notification panel's PanelController.
  PanelController* GetPanelController() const;

  // Returns the BalloonView object of the notification.
  BalloonViewImpl* GetBalloonView(BalloonCollectionImpl* collection,
                                  const Notification& notification);

  // True if the view is in visible in the ScrollView.
  bool IsVisible(const BalloonViewImpl* view) const;

 private:
  NotificationPanel* panel_;
  DISALLOW_COPY_AND_ASSIGN(NotificationPanelTester);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NOTIFICATIONS_NOTIFICATION_PANEL_H_
