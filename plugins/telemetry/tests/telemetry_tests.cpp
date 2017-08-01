/* This file is part of the KDE project
 *
   Copyright (C) 2017 Alexey Kapustin <akapust1n@mail.ru>


   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/
#include "telemetry_tests.h"
#include "kis_telemetry_instance.h"
#include "plugins/telemetry/telemetry_provider.h"
#include <QEventLoop>
#include <QJsonDocument>
#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <QTest>
#include <QVector>
#include <QtNetwork>

class Server : public QTcpServer {
    Q_OBJECT
public:
    explicit Server(QObject* parent = 0)
        : QTcpServer(parent)
    {
    }
    ~Server()
    {
        server_socket->close();
        server_socket->deleteLater();
    }

public Q_SLOTS:

    void tcpReady()
    {
        array = server_socket->read(server_socket->bytesAvailable());
        emit endRead();
    }

    bool start_listen(int port_no)
    {
        if (!this->listen(QHostAddress::Any, port_no)) {
            qDebug() << tr("Error!") << tr("Cannot listen to port %1").arg(port_no);
            return false;
        } else {
            return true;
        }
    }

    QByteArray resultRead()
    {
        return array;
    }

Q_SIGNALS:
    void endRead();

protected:
    void incomingConnection(qintptr descriptor)
    {

        server_socket = new QTcpSocket(this);
        if (!server_socket->setSocketDescriptor(descriptor)) {
            qDebug() << "Error set socket";
            return;
        }
        connect(server_socket, SIGNAL(readyRead()),
            this, SLOT(tcpReady()));
    }

private:
    QTcpSocket* server_socket;
    QByteArray array;
};

class TestTimeTickets : public QObject {
    Q_OBJECT

public:
    void notifyToolAcion(KisTelemetryInstance::Actions action, QString id)
    {

        switch (action) {
        case KisTelemetryInstance::ToolActivate: {
            id = KisTelemetryInstance::instance()->getToolId(id, KisTelemetryInstance::Activate);
            m_provider->putTimeTicket(id);
            break;
        }
        case KisTelemetryInstance::ToolDeactivate: {
            id = KisTelemetryInstance::instance()->getToolId(id, KisTelemetryInstance::Activate);
            m_provider->getTimeTicket(id);
            break;
        }
        case KisTelemetryInstance::ToolsStartUse: {
            id = KisTelemetryInstance::instance()->getToolId(id, KisTelemetryInstance::Use);
            m_provider->putTimeTicket(id);
            break;
        }
        case KisTelemetryInstance::ToolsStopUse: {
            id = KisTelemetryInstance::instance()->getToolId(id, KisTelemetryInstance::Use);
            m_provider->getTimeTicket(id);
            break;
        }
        default:
            break;
        }
    }
    TestTimeTickets()
    {
        m_provider = new TelemetryProvider();
    }
    ~TestTimeTickets()
    {
    }

    void testCountUse(int port)
    {
        Server server;
        server.start_listen(port);
        QEventLoop loop;
        connect(&server, SIGNAL(endRead()), &loop, SLOT(quit()));
        testUse("testTool2", m_countUse);
        QString localhost = QString("http://localhost:") + QString::number(port) + QString("/");
        m_provider->sendData("tools", localhost);
        loop.exec();
        loop.exec();
        QByteArray result = server.resultRead();
        checkCountUse(result, "Tool/Use/testTool2");
    }
    void testZeroUse(int port)
    {

        Server server;
        server.start_listen(port);
        QEventLoop loop;
        connect(&server, SIGNAL(endRead()), &loop, SLOT(quit()));
        testUse("testTool3", m_countUse);
        QString localhost = QString("http://localhost:") + QString::number(port) + QString("/");
        m_provider->sendData("tools", localhost);
        loop.exec();
        loop.exec();
        QByteArray result = server.resultRead();
        checkCountUse(result, "Tool/Use/testTool3");
    }

    void testTimeUse(int port)
    {
        Server server;
        server.start_listen(port);
        QEventLoop loop;
        m_time.push_back(2000);

        connect(&server, SIGNAL(endRead()), &loop, SLOT(quit()));
        uncorrectCountUse("Tool/Use/testTool3");
        QString localhost = QString("http://localhost:") + QString::number(port) + QString("/");
        m_provider->sendData("tools", localhost);
        loop.exec();
        loop.exec();

        QByteArray result = server.resultRead();
        checkCountUse(result, "Tool/Use/testTool3", 0);
    }

private:
    void testUse(QString toolID, int count)
    {
        for (int i = 0; i < count; i++) {
            notifyToolAcion(KisTelemetryInstance::ToolsStartUse, toolID);
            QTest::qWait(m_time[0]);
            notifyToolAcion(KisTelemetryInstance::ToolsStopUse, toolID);
        }
    }
    void uncorrectCountUse(QString toolID)
    {
        notifyToolAcion(KisTelemetryInstance::ToolsStartUse, toolID);
        QTest::qWait(m_time[0]);
    }

    void checkTimeUse(QByteArray request, QString toolName)
    {
        QJsonDocument document = QJsonDocument::fromJson(request);
        QJsonObject object = document.object();
        QJsonArray array = object["Tools"].toArray();

        for (int i = 0; i < array.size(); i++) {
            QJsonObject obj = array[i].toObject();
            if (toolName != obj["toolName"].toString())
                continue;
            QVERIFY(((obj["timeUseMSeconds"].toInt() + 10) > m_time[i]) && ((obj["timeUseMSeconds"].toInt() - 10) < m_time[i]));
        }
    }

    void checkCountUse(QByteArray request, QString toolName, int countUse = m_countUse)
    {
        QJsonDocument document = QJsonDocument::fromJson(request);
        QJsonObject object = document.object();
        QJsonArray array = object["Tools"].toArray();

        for (int i = 0; i < array.size(); i++) {
            QJsonObject obj = array[i].toObject();
            if (toolName != obj["toolName"].toString())
                continue;
            QVERIFY((countUse) == obj["countUse"].toInt());
        }
    }

private:
    KisTelemetryAbstract* m_provider;
    QVector<int> m_time;
    const static int m_countUse = 4;
};

void TelemetryTest::testTimeTicket()
{

    TestTimeTickets test1;
    test1.testTimeUse(23456);
}

void TelemetryTest::testCountUse()
{
    TestTimeTickets test2;
    test2.testCountUse(23457);
}

void TelemetryTest::testZeroUse()
{
    TestTimeTickets test3;
    test3.testCountUse(23458);
}
#include "telemetry_tests.moc"
QTEST_MAIN(TelemetryTest)
