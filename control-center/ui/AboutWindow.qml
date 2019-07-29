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
  visible: false
  property double scale: 1

  width: mainContents.implicitWidth + 24 * scale
  height: mainContents.implicitHeight + 24 * scale

  signal done

  Text {
    visible: false
    id: defaultText
    font.pixelSize: baseText.font.pixelSize * root.scale
  }

  RowLayout {

    ColumnLayout {
      id: mainContents
      Layout.leftMargin: 12
      Layout.rightMargin: 12

      Text {
        id: header
        Layout.leftMargin: defaultText.height * 2
        font.pixelSize: defaultText.font.pixelSize * 1.4
        text: "<b>Primo</b> - Privacy with <img src=\"qrc:///content/monero48.png\" height=" + font.pixelSize + ">onero"
        textFormat: Text.RichText
        topPadding: defaultText.font.pixelSize / 2
        bottomPadding: defaultText.font.pixelSize / 2
      }

      Text { } // spacer

      Text {
        text: "Primo allows the user to control which services to pay by mining Monero.<br>"
            + "It is typically used in conjunction to a browser extension.<br>"
        textFormat: Text.RichText
        font.pixelSize: baseText.font.pixelSize * root.scale
      }

      Text { } // spacer

      Text {
        text: "Author: selene"
        font.pixelSize: baseText.font.pixelSize * root.scale
      }
      TextEdit {
        text: "https://github.com/selene-kovri/primo-control-center"
        font.pixelSize: baseText.font.pixelSize * root.scale
        readOnly: true
        selectByMouse: true
      }
      Text {
        text: "License: GNU General Public License version 3 or later"
        font.pixelSize: baseText.font.pixelSize * root.scale
      }

      Text { } // spacer

      Text {
        text: "Donations welcome:"
        font.pixelSize: baseText.font.pixelSize * root.scale
      }
      TextEdit {
        text: "47DLxT6McbQ9oxZ9a2pMV7WS9h1UXf43w3GupUg5TpUfjiB25bh3BZdT9jscZckS36ZKKoAchxr5PTowNQZ8BkNA9knHuxj"
        readOnly: true
        selectByMouse: true
        font.pixelSize: baseText.font.pixelSize * root.scale * .8
      }

      Text { } // spacer

      Button {
        anchors.horizontalCenter: parent.horizontalCenter
        id: closeButton
        text: "OK"
        onClicked: done()
        style: ButtonStyle { label: Text { text: control.text; font.pixelSize: defaultText.font.pixelSize; horizontalAlignment: Text.AlignHCenter } }
      }
    }
  }
}
