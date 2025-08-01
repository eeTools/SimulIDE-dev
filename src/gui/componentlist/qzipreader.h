/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QZIPREADER_H
#define QZIPREADER_H

#ifndef QT_NO_TEXTODFWRITER


#include <QtCore/qfile.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QZipReaderPrivate;

class Q_AUTOTEST_EXPORT QZipReader
{
    public:
        QZipReader( const QString &fileName, QIODevice::OpenMode mode = QIODevice::ReadOnly );

        explicit QZipReader(QIODevice *device);
        ~QZipReader();

        bool isReadable() const;
        bool exists() const;

        struct Q_AUTOTEST_EXPORT FileInfo
        {
            FileInfo();
            FileInfo(const FileInfo &other);
            ~FileInfo();
            FileInfo &operator=(const FileInfo &other);
            QString filePath;
            uint isDir : 1;
            uint isFile : 1;
            uint isSymLink : 1;
            QFile::Permissions permissions;
            uint crc32;
            qint64 size;
            void *d;
        };

        QList<FileInfo> fileInfoList() const;
        int count() const;

        FileInfo entryInfoAt(int index) const;
        QByteArray fileData(const QString &fileName) const;
        bool extractAll(const QString &destinationDir) const;

        enum Status {
            NoError,
            FileReadError,
            FileOpenError,
            FilePermissionsError,
            FileError
        };

        Status status() const;

        void close();

    private:
        QZipReaderPrivate *d;
        Q_DISABLE_COPY(QZipReader)
};

QT_END_NAMESPACE

#endif // QT_NO_TEXTODFWRITER
#endif // QZIPREADER_H
