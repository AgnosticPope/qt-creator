/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "s60debuggerbluetoothstarter.h"

#include "bluetoothlistener.h"
#include "debuggerengine.h"
#include "trkdevice.h"

namespace Debugger {
namespace Internal {

S60DebuggerBluetoothStarter::S60DebuggerBluetoothStarter
        (const TrkDevicePtr& trkDevice, QObject *parent)
  : trk::AbstractBluetoothStarter(trkDevice, parent)
{
}

trk::BluetoothListener *S60DebuggerBluetoothStarter::createListener()
{
    DebuggerEngine *engine = 0; // FIXME: ABC
    trk::BluetoothListener *rc = new trk::BluetoothListener(engine);
    rc->setMode(trk::BluetoothListener::Listen);
    connect(rc, SIGNAL(message(QString)), engine, SLOT(showDebuggerOutput(QString)));
    connect(rc, SIGNAL(terminated()), engine, SLOT(startFailed()));
    return rc;
}

trk::PromptStartCommunicationResult
S60DebuggerBluetoothStarter::startCommunication(const TrkDevicePtr &trkDevice,
                                                 QWidget *msgBoxParent,
                                                 QString *errorMessage)
{
    // Bluetooth?
    if (trkDevice->serialFrame()) {
        BaseCommunicationStarter serialStarter(trkDevice);
        return trk::promptStartSerial(serialStarter, msgBoxParent, errorMessage);
    }
    S60DebuggerBluetoothStarter bluetoothStarter(trkDevice);
    return trk::promptStartBluetooth(bluetoothStarter, msgBoxParent, errorMessage);
}

} // namespace Internal
} // namespace Debugger
