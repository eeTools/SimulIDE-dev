/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QDomDocument>
#include <QFileDialog>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <qpoint.h>
#include <QPointF>
#include <cmath>

#include "mainwindow.h"
#include "utils.h"
#include "pin.h"

QMap<QString, double> multipliers = {{"p",1e-12},{"n",1e-9},{"µ",1e-6},{"m",1e-3},{"k",1e3},{"M",1e6},{"G",1e9},{"T",1e12}};

double getMultiplier( QString mult )
{
    mult = mult.remove(" ");
    mult = mult.left(1);
    double multiplier = multipliers.value( mult );
    if( multiplier == 0 ) multiplier = 1;
    return multiplier;
}

QString multToValStr( double value, QString mult )
{
    return QString::number( value*getMultiplier( mult ) );
}

QString val2hex( int d )
{
    QString h = toDigit( d );
    if( h.size()%2 ) h.prepend("0");
    return h;
}

QString toDigit( int d )
{
    QString Hex="0123456789ABCDEF";
    QString h = Hex.mid( d&0x0F, 1 );
    while( d > 0x0F ){
        d >>= 4;
        h.prepend( Hex.mid( d&0x0F, 1 ) );
    }
    return h;
}

QString decToBase( int value, int base, int digits )
{
    QString converted = "";
    for( int i=0; i<digits; i++ )
    {
        if( value >= base ) converted = toDigit( value%base ) + converted;
        else                converted = toDigit( value ) + converted;

        value = floor( value/base );
    }
    return converted;
}

//---------------------------------------------------

void MessageBoxNB( QString title, QString message )
{
    QMessageBox* msgBox = new QMessageBox( MainWindow::self() );
    msgBox->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
    msgBox->setStandardButtons( QMessageBox::Ok );
    msgBox->setWindowTitle( title );
    msgBox->setText( message );
    msgBox->setModal( false ); 
    msgBox->open();
}

//---------------------------------------------------

QString addQuotes( QString string )
{
    return "\""+string+"\"";
}
QString remQuotes( QString string )
{
    string = string.remove( 0, 1 );
    string = string.remove( string.size()-1, 1);
    return string;
}

QString getBareName( QString filepath ) // File name without extension
{
    return QFileInfo( filepath ).completeBaseName();
}
QString getFileName( QString filepath ) // Filename with extension
{
    return QFileInfo( filepath ).fileName();
}
QString getFileDir( QString filepath ) // File directory with end "/"
{
    return QFileInfo( filepath ).absolutePath()+"/";
}
QString getFileExt( QString filepath ) // File extension with "."
{
    return "."+QFileInfo( filepath ).suffix();
}
QString changeExt( QString filepath, QString ext )
{
    if( !ext.startsWith(".") ) ext.prepend(".");
    return getFileDir( filepath )+getBareName( filepath )+ext;
}

QString getDirDialog( QString msg, QString oldPath )
{
    QString path = QFileDialog::getExistingDirectory( nullptr
                         , msg
                         , oldPath
                         , QFileDialog::ShowDirsOnly
                         | QFileDialog::DontResolveSymlinks);

    if( !path.isEmpty() && !path.endsWith("/") && !path.endsWith("\\") ) path += "/";
    return path;
}

QString findFile( QString dir, QString fileName )
{
    QDir pathDir( dir );
    for( QFileInfo fileInfo : pathDir.entryInfoList() )
    {
        if( fileInfo.isFile() )
        {
            if( fileInfo.fileName() == fileName ) return fileInfo.absoluteFilePath();
        }
        else if( !fileInfo.fileName().endsWith(".") )
        {
            QString found = findFile( fileInfo.absoluteFilePath(), fileName );
            if( !found.isEmpty() ) return found;
        }
    }
    return "";
}

//---------------------------------------------------

QDomDocument fileToDomDoc( QString fileName, QString caller )
{
    QDomDocument domDoc;

    QFile file( fileName );
    if( !file.open( QFile::ReadOnly | QFile::Text) )
    {
        qDebug() << caller << "Error: Cannot read file:\n"+fileName+"\n"+file.errorString();
        return domDoc;
    }
    QString error;
    int errorLine=0;
    int errorColumn=0;
    if( !domDoc.setContent( &file, false, &error, &errorLine, &errorColumn ) )
    {
         qDebug() << caller << "Error: Cannot set file to DomDocument:\n"<<fileName<<"\nLine"<<errorLine<<errorColumn+"\n";
         qDebug() << error;
         domDoc.clear();
    }
    file.close();
    return domDoc;
}

QString fileToString( QString fileName, QString caller )
{
    QFile file( fileName );
    if (!file.open( QFile::ReadOnly | QFile::Text) )
    {
        qDebug() << caller << "Error: Cannot read file"<<Qt::endl<<fileName<<Qt::endl<<file.errorString();
        return "";
    }
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString text = in.readAll();
    file.close();

    return text;
}

QStringList fileToStringList( QString fileName, QString caller )
{
    return fileToString( fileName, caller ).split('\n');
}

QByteArray fileToByteArray( QString fileName, QString caller )
{
    QByteArray ba;

    QFile file(fileName);
    if( !file.open( QFile::ReadOnly ) )
    {
        qDebug() << caller << "Error: Cannot read file"<<Qt::endl<<fileName<<Qt::endl<<file.errorString();
        return ba;
    }
    ba = file.readAll();
    file.close();

    return ba;
}

//--------------------------------------------------------------

int roundDown( int x, int roundness )
{
    if( x < 0 ) return (x-roundness+1) / roundness;
    else        return (x / roundness);
}

int snapToGrid4( int x )     { return roundDown( x+2, 4 )*4; }
int snapToGrid8( int x ) { return roundDown( x+4, 8 )*8; }

QPointF toCompGrid( QPointF point )
{
    int valor;
    valor = snapToGrid8( (int)point.x() );
    point.rx() = (float)valor;
    valor = snapToGrid8( (int)point.y() );
    point.ry() = (float)valor;
    return point;
}
QPointF toGrid( QPointF point )
{
    int valor;
    valor = snapToGrid4( (int)point.x() );
    point.rx() = (float)valor;
    valor = snapToGrid4( (int)point.y() );
    point.ry() = (float)valor;
    return point;
}
QPoint toGrid( QPoint point )
{
    int valor;
    valor = snapToGrid4( (int)point.x() );
    point.rx() = valor;
    valor = snapToGrid4( (int)point.y() );
    point.ry() = valor;
    return point;
}

bool lessPinX( Pin* pinA, Pin* pinB )
{
    return pinA->x() < pinB->x();
}

bool lessPinY( Pin* pinA, Pin* pinB )
{
    return pinA->y() < pinB->y();
}

/*QPointF getPointF( QString p )
{
    QStringList plist = p.split(",");
    QPointF point( plist.first().toDouble(), plist.last().toDouble() );
    return point;
}
QString getStrPointF( QPointF p )
{
    return QString::number(p.x())+","+QString::number(p.y());
}*/
