/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangutils.h"

#include <clang-c/Index.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <cpptools/baseeditordocumentparser.h>
#include <cpptools/cppprojects.h>
#include <cpptools/cppworkingcopy.h>

#include <utils/qtcassert.h>

#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QSet>
#include <QString>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace Core;
using namespace CppTools;

namespace ClangCodeModel {
namespace Utils {

Q_LOGGING_CATEGORY(verboseRunLog, "qtc.clangcodemodel.verboserun")

UnsavedFiles createUnsavedFiles(WorkingCopy workingCopy)
{
    // TODO: change the modelmanager to hold one working copy, and amend it every time we ask for one.
    // TODO: Reason: the UnsavedFile needs a QByteArray.

    QSet< ::Utils::FileName> modifiedFiles;
    foreach (IDocument *doc, DocumentManager::modifiedDocuments())
        modifiedFiles.insert(doc->filePath());

    UnsavedFiles result;
    QHashIterator< ::Utils::FileName, QPair<QByteArray, unsigned> > wcIter = workingCopy.iterator();
    while (wcIter.hasNext()) {
        wcIter.next();
        const ::Utils::FileName &fileName = wcIter.key();
        if (modifiedFiles.contains(fileName) && QFile(fileName.toString()).exists())
            result.insert(fileName.toString(), wcIter.value().first);
    }

    return result;
}

/**
 * @brief Creates list of command-line arguments required for correct parsing
 * @param pPart Null if file isn't part of any project
 * @param fileName Path to file, non-empty
 */
QStringList createClangOptions(const ProjectPart::Ptr &pPart, const QString &fileName)
{
    ProjectFile::Kind fileKind = ProjectFile::Unclassified;
    if (!pPart.isNull())
        foreach (const ProjectFile &file, pPart->files)
            if (file.path == fileName) {
                fileKind = file.kind;
                break;
            }
    if (fileKind == ProjectFile::Unclassified)
        fileKind = ProjectFile::classify(fileName);

    return createClangOptions(pPart, fileKind);
}

static QString getResourceDir()
{
    QDir dir(ICore::instance()->resourcePath() + QLatin1String("/cplusplus/clang/") +
             QLatin1String(CLANG_VERSION) + QLatin1String("/include"));
    if (!dir.exists() || !QFileInfo(dir, QLatin1String("stdint.h")).exists())
        dir = QDir(QLatin1String(CLANG_RESOURCE_DIR));
    return dir.canonicalPath();
}

static bool maybeIncludeBorlandExtensions()
{
    return
#if defined(CINDEX_VERSION) // clang 3.2 or higher
        true;
#else
        false;
#endif
}

class LibClangOptionsBuilder : public CompilerOptionsBuilder
{
public:
    static QStringList build(const ProjectPart::Ptr &pPart, ProjectFile::Kind fileKind)
    {
        if (pPart.isNull())
            return QStringList();

        LibClangOptionsBuilder optionsBuilder(pPart);

        if (verboseRunLog().isDebugEnabled())
            optionsBuilder.add(QLatin1String("-v"));

        optionsBuilder.addLanguageOption(fileKind);
        optionsBuilder.addOptionsForLanguage(maybeIncludeBorlandExtensions());
        optionsBuilder.addToolchainAndProjectDefines();

        static const QString resourceDir = getResourceDir();
        if (!resourceDir.isEmpty()) {
            optionsBuilder.add(QLatin1String("-nostdlibinc"));
            optionsBuilder.add(QLatin1String("-I") + resourceDir);
            optionsBuilder.add(QLatin1String("-undef"));
        }

        optionsBuilder.addHeaderPathOptions();

        // Inject header file
        static const QString injectedHeader = ICore::instance()->resourcePath()
                + QLatin1String("/cplusplus/qt%1-qobjectdefs-injected.h");

//        if (pPart->qtVersion == ProjectPart::Qt4) {
//            builder.addOption(QLatin1String("-include"));
//            builder.addOption(injectedHeader.arg(QLatin1Char('4')));
//        }

        if (pPart->qtVersion == ProjectPart::Qt5) {
            optionsBuilder.add(QLatin1String("-include"));
            optionsBuilder.add(injectedHeader.arg(QLatin1Char('5')));
        }

        if (!pPart->projectConfigFile.isEmpty()) {
            optionsBuilder.add(QLatin1String("-include"));
            optionsBuilder.add(pPart->projectConfigFile);
        }

        optionsBuilder.add(QLatin1String("-fmessage-length=0"));
        optionsBuilder.add(QLatin1String("-fdiagnostics-show-note-include-stack"));
        optionsBuilder.add(QLatin1String("-fmacro-backtrace-limit=0"));
        optionsBuilder.add(QLatin1String("-fretain-comments-from-system-headers"));
        // TODO: -Xclang -ferror-limit -Xclang 0 ?

        return optionsBuilder.options();
    }

private:
    LibClangOptionsBuilder(const CppTools::ProjectPart::Ptr &projectPart)
        : CompilerOptionsBuilder(projectPart)
    {
    }

    bool excludeHeaderPath(const QString &path) const override
    {
        return path.contains(QLatin1String("lib/gcc/i686-apple-darwin"));
    }
};

/**
 * @brief Creates list of command-line arguments required for correct parsing
 * @param pPart Null if file isn't part of any project
 * @param fileKind Determines language and source/header state
 */
QStringList createClangOptions(const ProjectPart::Ptr &pPart, ProjectFile::Kind fileKind)
{
    return LibClangOptionsBuilder::build(pPart, fileKind);
}

/// @return Option to speed up parsing with precompiled header
QStringList createPCHInclusionOptions(const QStringList &pchFiles)
{
    QStringList opts;

    foreach (const QString &pchFile, pchFiles) {
        if (QFile(pchFile).exists()) {
            opts += QLatin1String("-include-pch");
            opts += pchFile;
        }
    }

    return opts;
}

/// @return Option to speed up parsing with precompiled header
QStringList createPCHInclusionOptions(const QString &pchFile)
{
    return createPCHInclusionOptions(QStringList() << pchFile);
}

ProjectPart::Ptr projectPartForFile(const QString &filePath)
{
    if (CppTools::BaseEditorDocumentParser *parser = CppTools::BaseEditorDocumentParser::get(filePath))
        return parser->projectPart();
    return ProjectPart::Ptr();
}

bool isProjectPartValid(const ProjectPart::Ptr projectPart)
{
    if (projectPart)
        return CppModelManager::instance()->projectPartForProjectFile(projectPart->projectFile);
    return false;
}

QString projectFilePathForFile(const QString &filePath)
{
    const ProjectPart::Ptr projectPart = projectPartForFile(filePath);

    if (isProjectPartValid(projectPart))
        return projectPart->projectFile; // OK, Project Part is still loaded
    return QString();
}

} // namespace Utils
} // namespace Clang
