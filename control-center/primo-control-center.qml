/*  Primo - Privacy with Monero
 *
 *  Copyright (C) 2019 selene
 *
 *  This file is part of Primo.
 *
 *  Primo is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Primo is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Primo.  If not, see <https://www.gnu.org/licenses/>.
 */

import QtQuick 2.9
import QtQuick.Window 2.1
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.0
import Model 1.0
import "ui" 1.0

ApplicationWindow {
  id: root
  visible: true
  width: header.implicitWidth * 2.5

  property string demo_url: "http://127.0.0.1/primo/"
  property string demo_mining_page: "http://127.0.0.1/primo/primo-mining.html"
  property bool debug: demo_url.indexOf("127.0.0.1") >= 0

  property bool showAndTellStarted: false
  property bool skipShowAndTell: false
  property int max_target: 99999999
  property string orange: "#ff6c3c"
  property string lightblue: "#c2fff6"
  property double scale: 1.15

  // function helpers
  function max(a, b) {
    return a > b ? a : b;
  }
  function getWidgetToolTipPosition(w, dx, dy) {
    var pos = w.mapToGlobal(0, 0)
    pos.x += dx
    pos.y += dy
    pos.x = max(pos.x, 0)
    pos.y = max(pos.y, 0)
    return pos
  }
  function showAndTell() {
    if (showAndTellStarted)
      return
    showAndTellStarted = true
    skipShowAndTell = false
    showAndTellIntro()
  }
  function showAndTellIntro()
  {
    tooltip.done.connect(showAndTellBalance)
    tooltip.show(getWidgetToolTipPosition(table, cEnabled.width, defaultText.height * 2),
      "A new site rquested payment and was added to the list.<br>" +
      "Primo will query its info, such as name and cost.")
  }
  function showAndTellBalance()
  {
    tooltip.done.disconnect(showAndTellBalance)
    if (skipShowAndTell) return;
    tooltip.done.connect(showAndTellTarget)
    tooltip.show(getWidgetToolTipPosition(table, cEnabled.width + cName.width, defaultText.height * 2),
      "Your balance for this site starts at zero.<br>" +
      "Mining will slowly bring it up.<br>" +
      "Browsing this site will slowly bring it down.")
  }
  function showAndTellTarget()
  {
    tooltip.done.disconnect(showAndTellTarget)
    if (skipShowAndTell) return;
    tooltip.done.connect(showAndTellMining)
    tooltip.show(getWidgetToolTipPosition(table, cEnabled.width + cName.width + cBalance.width, defaultText.height * 2),
      "To mine, set a target balance for this site.<br>" +
      "Primo will mine until your balance reaches the target.<br>" +
      "If your balance falls below the target,<br>" +
      "Primo will mine some more to top it up, etc.")
  }
  function showAndTellMining()
  {
    tooltip.done.disconnect(showAndTellMining)
    if (skipShowAndTell) return;
    tooltip.done.connect(showAndTellPause)
    tooltip.show(getWidgetToolTipPosition(mining, 0, mining.height),
      "At the top, you can see when your computer is mining.")
  }
  function showAndTellPause()
  {
    tooltip.done.disconnect(showAndTellPause)
    if (skipShowAndTell) return;
    tooltip.done.connect(showAndTellAllow)
    tooltip.show(getWidgetToolTipPosition(pause, 0, pause.height),
      "At any point, you can pause mining entirely<br>" +
      "by pressing the pause button.")
  }
  function showAndTellAllow()
  {
    tooltip.done.disconnect(showAndTellAllow)
    if (skipShowAndTell) return;
    tooltip.done.connect(showAndTellForget)
    tooltip.show(getWidgetToolTipPosition(table, 0, defaultText.height * 2),
      "Or you can select which sites you wish to mine for<br>" +
      "and which not to. You can toggle them at any time.")
  }
  function showAndTellForget()
  {
    tooltip.done.disconnect(showAndTellForget)
    if (skipShowAndTell) return;
    tooltip.done.connect(showAndTellForgetAll)
    tooltip.show(getWidgetToolTipPosition(table, cEnabled.width + cName.width + cBalance.width + cTarget.width, defaultText.height * 2),
      "You can remove a site when you're done browsing.<br>" +
      "This will zero any balance for this site,<br>" +
      "but is better for your privacy since it cannot<br>" +
      "recognize you by your Primo account.")
  }
  function showAndTellForgetAll()
  {
    tooltip.done.disconnect(showAndTellForgetAll)
    if (skipShowAndTell) return;
    tooltip.done.connect(showAndTellDone)
    tooltip.show(getWidgetToolTipPosition(forgetAll, 0, forgetAll.height),
      "You can also remove all sites at once.<br>" +
      "It is recommended to do this every time you start browsing again,<br>" +
      "so sites can't track you day to day.")
  }
  function showAndTellDone()
  {
    tooltip.done.disconnect(showAndTellDone)
    if (skipShowAndTell) return;
    tooltip.show(null,
      "Primo gives you a new way to support websites<br>" +
      "while staying safe from malware laden privacy invading ads,<br>" +
      "and it also supports the Monero network.")
  }
  function about()
  {
    aboutWindow.x = root.x + root.width / 2 - aboutWindow.width / 2
    aboutWindow.y = root.y + root.height / 2 - aboutWindow.height / 2
    aboutWindow.visible = true
  }

  // size helpers
  Text {
    visible: false
    id: baseText
  }
  Text {
    visible: false
    id: defaultText
    font.pixelSize: baseText.font.pixelSize * root.scale
  }
  Text {
    visible: false
    id: maxTargetText
    text: max_target
    font.pixelSize: defaultText.font.pixelSize
  }
  Text {
    visible: false
    id: maxNameText
    text: "The Imperial Times of New Somewheria"
    font.pixelSize: defaultText.font.pixelSize
  }
  Text {
    visible: false
    text: " Allow "
    id: defaultTextAllow
    font.pixelSize: defaultText.font.pixelSize
  }
  Text {
    visible: false
    text: "5000.00"
    id: defaultTextMinutesPerRequest
    font.pixelSize: defaultText.font.pixelSize
  }
  Button {
    visible: false
    text: "Forget"
    id: defaultButtonForget
  }
  CheckBox {
    visible: false
    id: defaultCheckbox
  }
  FontLoader {
    id: fontAwesome
    source: "./content/fontawesome.otf"
  }


  // UI
  ColumnLayout {
    id: main
    anchors.fill: parent

    RowLayout {
      Layout.fillWidth: true
      ColumnLayout {
        Text {
          id: header
          Layout.leftMargin: defaultText.height * 2
          font.pixelSize: defaultText.font.pixelSize * 1.4
          //text: "<b>Primo</b> - Privacy with \uf3d0onero"
          //text: "<b>Primo</b> - Privacy with <img src=\"qrc:///content/monero48.png\" height=" + font.pixelSize + ">onero"
          text: "<b>Primo</b> - Privacy with <img src=\"qrc:///content/monero48.png\" height=" + font.pixelSize + ">onero"
          textFormat: Text.RichText
          topPadding: defaultText.font.pixelSize / 2
          bottomPadding: defaultText.font.pixelSize / 2
        }
      }

      Button {
        text: "Show and tell"
        visible: root.debug
        onClicked: { showAndTellStarted = false; showAndTell(); }
        style: ButtonStyle { label: Text { text: control.text; font.pixelSize: defaultText.font.pixelSize; horizontalAlignment: Text.AlignHCenter } }
      }

      Item {
        Layout.fillWidth: true
      }

      Button {
        text: "About"
        onClicked: { about(); }
        style: ButtonStyle { label: Text { text: control.text; font.pixelSize: defaultText.font.pixelSize; horizontalAlignment: Text.AlignHCenter } }
      }

      Text {
        rightPadding: defaultText.font.pixelSize / 2
      }
    }

    RowLayout {
      Layout.fillWidth: true
      opacity: table.enabled ? 1 : 0
      Layout.leftMargin: defaultText.height / 2
      Layout.rightMargin: Layout.leftMargin
      Button {
        id: pause
        text: {
          miner.paused ? "Unpause" : "Pause";
        }
        onClicked: miner.paused = !miner.paused
        style: ButtonStyle { label: Text { text: control.text; font.pixelSize: defaultText.font.pixelSize; horizontalAlignment: Text.AlignHCenter } }
      }
      Text {
        id: mining
        text: {
          if (miner.hashRate > 0)
            return qsTr("Mining for %2 at %1 H/s").arg(miner.hashRate.toFixed(1)).arg(miner.miningName)
          else
            return "Not mining"
        }
        font.pixelSize: defaultText.font.pixelSize
      }
      Rectangle {
        Layout.fillWidth: true
      }
      Button {
        id: forgetAll
        text: "Forget all"
        onClicked: db.deleteAllEntries()
        style: ButtonStyle { label: Text { text: control.text; font.pixelSize: defaultText.font.pixelSize; horizontalAlignment: Text.AlignHCenter } }
      }
    }

    TableView {
      id: table
      model: dbmodel
      enabled: (model && (model.numRows > 0)) ? 1 : 0
      opacity: enabled ? 1 : 0
      Layout.fillHeight: true
      Layout.fillWidth: true
      alternatingRowColors: true

      rowDelegate: Rectangle { height: defaultText.height * 1.1 }

      TableViewColumn {
        id: cEnabled
        role: "enabled"
        width: defaultTextAllow.height * 1.2
        // checkbox does not work well, it "bleeds" over other rows, so let's cheat
        delegate: Button {
          text: { if (styleData.value) return "\uf046"; else return "\uf096"; }
          onClicked: {
            db.setEnabled(styleData.row, !styleData.value)
          }
          style: ButtonStyle { background: Rectangle {} label: Text { text: control.text; font.pixelSize: defaultText.font.pixelSize; horizontalAlignment: Text.AlignHCenter } }
        }
      }
      TableViewColumn {
        id: cMining
        width: defaultText.height * 1.2
        role: "mining_page"
        delegate: RowLayout {
          Item {
            width: defaultText.height
            height: defaultText.height

            // Spinner
            Image {
              id: spinner
              source: "qrc:///content/spinning-light3.png"
              anchors.fill: parent
              fillMode: Image.PreserveAspectFit
              visible: miner.miningPage == styleData.value && miner.hashRate > 0
              RotationAnimator {
                id: spinnerAnimation
                target: spinner
                from: 360
                loops: Animation.Infinite
                to: 0
                duration: 850
                running: spinner.visible
              }
              onVisibleChanged: { if (visible) spinnerAnimation.restart(); }
            }
          }
        }
      }
      TableViewColumn {
        id: cName
        role: "name_or_page"
        title: "Name"
        width: maxNameText.width * 1.1
        delegate: Text { text: styleData.value; font.pixelSize: defaultText.font.pixelSize }
      }
      TableViewColumn {
        id: cBalance
        role: "balance_string"
        title: "Balance"
        width: maxTargetText.width * 1.1
        delegate: Text { text: styleData.value; font.pixelSize: defaultText.font.pixelSize }
      }
      TableViewColumn {
        id: cTarget
        role: "target_string"
        title: "Target"
        width: maxTargetText.width * 1.1
        delegate: TextInput {
          text: styleData.value
          validator: IntValidator {
            bottom: 0
            top: max_target
          }
          selectByMouse: true
          onAccepted: {
            db.setTarget(styleData.row, text)
            focus = false
          }
          font.pixelSize: defaultText.font.pixelSize
        }
      }
      TableViewColumn {
        id: cForget
        width: defaultButtonForget.width * 1.1
        title: "Forget"
        delegate: Button {
          text: "\uf014"
          onClicked: {
            db.deleteEntry(styleData.row)
          }
          style: ButtonStyle { background: Rectangle {} label: Text { text: control.text; font.pixelSize: defaultText.font.pixelSize; horizontalAlignment: Text.AlignHCenter } }
        }
      }
      TableViewColumn {
        id: cMinutesPerRequest
        role: "minutes_per_request_string"
        width: defaultTextMinutesPerRequest.width
        title: "Cost"
        delegate: Text {
          text: styleData.value
          horizontalAlignment: Text.AlignHCenter
          font.pixelSize: defaultText.font.pixelSize
        }
      }
    }

    Rectangle {
      property int x2: table.x
      property int y2: table.y
      property int w2: table.width
      property int h2: table.height
      Text {
        id: tablePlaceholder
        property string demo_url: root.demo_url ? root.demo_url : ""
        property string extra_line: { if (demo_url && is_native_messaging) return "Try one here: <a href=\"+demo_url+\">"+demo_url+"</a>"; else return ""; }
        x: parent.x2
        y: parent.y2 + parent.h2 / 2.5 - height / 2
        width: parent.w2
        height: parent.h2
        font.pixelSize: defaultText.font.pixelSize * 1.4
        opacity: table.enabled ? 0 : 1
        text: "No sites yet<br>Browse to a Primo enabled website to fill this list<br>" + extra_line
        textFormat: Text.RichText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        clip:false
        onLinkActivated: {
          handler.requestURL(demo_url)
        }
      }
    }
  }

  ToolTip {
    id: tooltip
    target: main.parent
    color: "#c2fff6"
    arrowImage: "qrc:///content/arrow4.png"
    scale: root.scale
    showButtons: true
    onButtonSkip: { root.skipShowAndTell = true; hide(); }
    onButtonNext: { hide(); }
    onButtonDone: { hide(); }
  }

  onXChanged: tooltip.updatePosition()
  onYChanged: tooltip.updatePosition()

  AboutWindow {
    id: aboutWindow
    visible: false
    onDone: aboutWindow.visible = false
    scale: root.scale
  }

  Connections {
    target: db
    onNewSite: function(mining_page, name) { if (root.demo_mining_page && mining_page == root.demo_mining_page) showAndTell(); }
  }

}
