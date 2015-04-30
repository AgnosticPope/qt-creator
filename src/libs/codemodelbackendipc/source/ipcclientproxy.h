/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CODELMODELBACKEND_IPCCLIENTPROXY_H
#define CODELMODELBACKEND_IPCCLIENTPROXY_H

#include <QtGlobal>

#include <memory>

#include "codemodelbackendipc_global.h"
#include "ipcclientinterface.h"

#include "writecommandblock.h"
#include "readcommandblock.h"

QT_BEGIN_NAMESPACE
class QLocalServer;
class QIODevice;
QT_END_NAMESPACE

namespace CodeModelBackEnd {

class CMBIPC_EXPORT IpcClientProxy : public IpcClientInterface
{
public:
    explicit IpcClientProxy(IpcServerInterface *server, QIODevice *ioDevice);
    IpcClientProxy(const IpcClientProxy&) = delete;
    const IpcClientProxy &operator =(const IpcClientProxy&) = delete;

    IpcClientProxy(IpcClientProxy&&other);
    IpcClientProxy &operator =(IpcClientProxy&&other);

    void alive() override;
    void echo(const EchoCommand &command) override;
    void codeCompleted(const CodeCompletedCommand &command) override;
    void translationUnitDoesNotExist(const TranslationUnitDoesNotExistCommand &command) override;
    void projectDoesNotExist(const ProjectDoesNotExistCommand &command) override;

    void readCommands();

    bool isUsingThatIoDevice(QIODevice *ioDevice) const;

private:
    CodeModelBackEnd::WriteCommandBlock writeCommandBlock;
    CodeModelBackEnd::ReadCommandBlock readCommandBlock;
    IpcServerInterface *server;
    QIODevice *ioDevice;
};

} // namespace CodeModelBackEnd

#endif // CODELMODELBACKEND_IPCCLIENTPROXY_H
