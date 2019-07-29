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

Window {
    id: root
    flags: Qt.ToolTip | Qt.FramelessWindowHint | Qt.WA_TranslucentBackground
    visible: false
    width: tooltipContents.implicitWidth + 24 * scale
    height: tooltipContents.implicitHeight + 24 * scale

    property Item target: null
    property string arrowImage: ""
    property string text: ""
    property bool arrowVisible: true
    property var previousTargetPosition: 0
    property string color: "lightgrey"
    property int timeout: 30000
    property double scale: 1
    property bool showButtons: false
    signal buttonNext
    signal buttonSkip
    signal buttonDone
    signal done

    function show(pos, text)
    {
      arrowVisible = !!pos
      root.text = text
      previousTargetPosition = target.mapToGlobal(0, 0)
      if (pos)
      {
        x = pos.x
        y = pos.y
      }
      else
      {
        x = previousTargetPosition.x + target.width / 2 - root.width / 2
        y = previousTargetPosition.y + target.height / 2 - root.height / 2
      }
      visible = true
      tooltipTimer.restart()
    }
    function hide() {
      tooltipTimer.stop()
      text = ""
      done()
      if (text == "")
        visible = false
    }
    function updatePosition()
    {
      if (!target)
        return
      var target_pos = target.mapToGlobal(0, 0)
      var newRootX = target_pos.x
      var newRootY = target_pos.y
      var dx = newRootX - previousTargetPosition.x
      var dy = newRootY - previousTargetPosition.y
      previousTargetPosition.x = newRootX
      previousTargetPosition.y = newRootY
      x += dx
      y += dy
      if (visible)
      {
        visible = false
        visible = true
      }
    }

    Text {
      id: baseText
      visible: false
    }
    Text {
      id: defaultText
      visible: false
      font.pixelSize: baseText.font.pixelSize * root.scale
    }

    Item {
      id: toolTipItem
      objectName: "toolTipItem"
      anchors.fill: parent
      anchors.centerIn: parent

      MouseArea {
        anchors.fill: parent
        anchors.centerIn: parent
        hoverEnabled: true
        onClicked: { root.hide() }
      }
      ColumnLayout {
        id: toolTipColumnLayout
        anchors.fill: parent
        anchors.centerIn: parent
        Item {
          id: arrow
          width: defaultText.height * 2
          height: defaultText.height
          visible: root.arrowVisible
          z: 1
          Image {
            source: root.arrowImage
            anchors.fill: parent
            anchors.centerIn: parent
            anchors.bottomMargin: -2
            anchors.leftMargin: defaultText.height
          }
        }
        Rectangle {
          id: mainToolTipRectangle
          color: root.color
          anchors.top: arrow.bottom
          anchors.left: toolTipColumnLayout.left
          anchors.right: toolTipColumnLayout.right
          anchors.bottom: toolTipColumnLayout.bottom
          border.width: 2
          border.color: "darkgrey"
          radius: 4

          ColumnLayout {
            id: tooltipContents
            anchors.fill: parent

            Text {
              Layout.topMargin: defaultText.height / 2
              Layout.bottomMargin: defaultText.height / 2
              id: tooltipText
              text: root.text
              anchors.horizontalCenter: parent.horizontalCenter
              textFormat: Text.RichText
              verticalAlignment: Text.AlignVCenter
              font.pixelSize: defaultText.font.pixelSize
            }
            RowLayout {
              anchors.left: tooltipText.left
              visible: root.showButtons
              Layout.bottomMargin: defaultText.height / 2
	      Button {
                text: "Skip"
                visible: root.arrowVisible
                onClicked: { root.buttonSkip(); }
                style: ButtonStyle { label: Text { text: control.text; font.pixelSize: defaultText.font.pixelSize; horizontalAlignment: Text.AlignHCenter } }
              }
	      Button { text: "Next"
                visible: root.arrowVisible
                onClicked: { root.buttonNext(); }
                style: ButtonStyle { label: Text { text: control.text; font.pixelSize: defaultText.font.pixelSize; horizontalAlignment: Text.AlignHCenter } }
              }
	      Button { text: "Done"
                visible: !root.arrowVisible
                onClicked: { root.buttonDone(); }
                style: ButtonStyle { label: Text { text: control.text; font.pixelSize: defaultText.font.pixelSize; horizontalAlignment: Text.AlignHCenter } }
              }
            }
          }
        }
      }
      Timer {
        id: tooltipTimer
        interval: root.timeout
        running: false
        repeat: false
        onTriggered: { root.hide(); }
      }
    }
}
