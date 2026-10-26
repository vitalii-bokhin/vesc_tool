/*
    Copyright 2016 - 2025 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include "pagecontrollersetup.h"
#include "ui_pagecontrollersetup.h"
#include <QWidget>
#include <QMessageBox>
#include <QFileDialog>

PageControllerSetup::PageControllerSetup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageControllerSetup)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;
}

PageControllerSetup::~PageControllerSetup()
{
    delete ui;
}

VescInterface *PageControllerSetup::vesc() const
{
    return mVesc;
}

void PageControllerSetup::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ui->detectFoc->setVesc(mVesc);
        if (ui->detectFoc) {
            ui->detectFoc->setHideApplyButton(true);
        }
        reloadParams();
    }
}

void PageControllerSetup::reloadParams()
{
    if (mVesc) {
        ui->generalTab->clearParams();
        ui->generalTab->addParamSubgroup(mVesc->mcConfig(), "foc", "general");
    }
}

void PageControllerSetup::on_loadMotorConfButton_clicked()
{
    if (!mVesc) {
        QMessageBox::warning(this, tr("Load Motor Configuration"),
                             tr("Not connected to the VESC."));
        return;
    }

    QString path;
    path = QFileDialog::getOpenFileName(this,
                                        tr("Choose motor configuration file to load"),
                                        QString(),
                                        tr("Xml files (*.xml)"));

    if (path.isNull()) {
        return;
    }

    bool res = mVesc->mcConfig()->loadXml(path, "MCConfiguration");

    if (res) {
        mVesc->emitStatusMessage(tr("Loaded motor configuration"), true);
        mVesc->commands()->setMcconf();
    } else {
        QMessageBox::warning(this, tr("Load motor configuration"),
                             tr("Could not load motor configuration:<BR>"
                                "%1").arg(mVesc->mcConfig()->xmlStatus()));
    }
}

void PageControllerSetup::on_loadAppConfButton_clicked()
{
    if (!mVesc) {
        QMessageBox::warning(this, tr("Load App Configuration"),
                             tr("Not connected to the VESC."));
        return;
    }

    QString path;
    path = QFileDialog::getOpenFileName(this,
                                        tr("Choose app configuration file to load"),
                                        QString(),
                                        tr("Xml files (*.xml)"));

    if (path.isNull()) {
        return;
    }

    bool res = mVesc->appConfig()->loadXml(path, "APPConfiguration");

    if (res) {
        mVesc->emitStatusMessage(tr("Loaded app configuration"), true);
        mVesc->commands()->setAppConf();
    } else {
        QMessageBox::warning(this, tr("Load app configuration"),
                             tr("Could not load app configuration:<BR>"
                                "%1").arg(mVesc->appConfig()->xmlStatus()));
    }
}
