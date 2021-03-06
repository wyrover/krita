/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_grid_config_test.h"

#include <QTest>
#include "kis_grid_config.h"
#include "kis_guides_config.h"
#include <QDomDocument>
#include <QDomElement>


void KisGridConfigTest::testGridConfig()
{
    KisGridConfig config;
    config.setSpacing(QPoint(10,13));
    config.setOffset(QPoint(13,14));
    config.setOffsetAspectLocked(false);
    config.setSubdivision(4);

    QVERIFY(!config.isDefault());

    QDomDocument doc;
    QDomElement root = doc.createElement("TestXMLRoot");
    doc.appendChild(root);
    QDomElement el = config.saveDynamicDataToXml(doc, "test_tag");
    root.appendChild(el);

    QByteArray b = doc.toByteArray(4);
    //printf(b.data());

    KisGridConfig config2;
    QVERIFY(config2.isDefault());
    QVERIFY(config2.loadDynamicDataFromXml(el));

    QCOMPARE(config2, config);
    QVERIFY(!config2.isDefault());
}

void KisGridConfigTest::testGuidesConfig()
{
    KisGuidesConfig config;
    config.setShowGuides(true);
    config.setLockGuides(true);
    config.setSnapToGuides(true);

    config.addGuideLine(Qt::Horizontal, 100.0);
    config.addGuideLine(Qt::Horizontal, 200.0);

    config.addGuideLine(Qt::Vertical, 300.0);
    config.addGuideLine(Qt::Vertical, 400.0);

    QVERIFY(config.hasGuides());

    QDomDocument doc;
    QDomElement root = doc.createElement("TestXMLRoot");
    doc.appendChild(root);
    QDomElement el = config.saveToXml(doc, "test_tag");
    root.appendChild(el);

    QByteArray b = doc.toByteArray(4);
    //printf(b.data());

    KisGuidesConfig config2;
    QVERIFY(!config2.hasGuides());
    QVERIFY(config2.loadFromXml(el));

    QCOMPARE(config2, config);
    QVERIFY(config2.hasGuides());
}

QTEST_MAIN(KisGridConfigTest)
