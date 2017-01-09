// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QtGui>
#include "citra_qt/configure_camera.h"
#include "core/settings.h"
#include "ui_configure_camera.h"

ConfigureCamera::ConfigureCamera(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureCamera>()) {
    ui->setupUi(this);

    ui->camera_outer_right_name->clear();
    ui->camera_outer_right_name->addItem("blank");
    ui->camera_outer_right_name->addItem("image");

    ui->camera_outer_left_name->clear();
    ui->camera_outer_left_name->addItem("blank");
    ui->camera_outer_left_name->addItem("image");

    ui->camera_inner_name->clear();
    ui->camera_inner_name->addItem("blank");
    ui->camera_inner_name->addItem("image");

    this->setConfiguration();
}

ConfigureCamera::~ConfigureCamera() {}

void ConfigureCamera::setConfiguration() {
    using namespace Service::CAM;

    // outer right camera
    if (ui->camera_outer_right_name->itemText(0).toStdString() == Settings::values.camera_name[OuterRightCamera]) {
        ui->camera_outer_right_name->setCurrentIndex(0);
    } else {
        ui->camera_outer_right_name->setCurrentIndex(1);
    }
    ui->camera_outer_right_config->setText(
        QString::fromStdString(Settings::values.camera_config[OuterRightCamera]));

    // outer left camera
    if (ui->camera_outer_left_name->itemText(0).toStdString() ==
        Settings::values.camera_name[OuterLeftCamera]) {
        ui->camera_outer_left_name->setCurrentIndex(0);
    } else {
        ui->camera_outer_left_name->setCurrentIndex(1);
    }
    ui->camera_outer_left_config->setText(
        QString::fromStdString(Settings::values.camera_config[OuterLeftCamera]));

    // inner camera
    if (ui->camera_inner_name->itemText(0).toStdString() ==
        Settings::values.camera_name[InnerCamera]) {
        ui->camera_inner_name->setCurrentIndex(0);
    } else {
        ui->camera_inner_name->setCurrentIndex(1);
    }
    ui->camera_inner_config->setText(
        QString::fromStdString(Settings::values.camera_config[InnerCamera]));
}


void ConfigureCamera::applyConfiguration() {
    using namespace Service::CAM;
    // outer right camera
    Settings::values.camera_name[OuterRightCamera] =
        ui->camera_outer_right_name->itemText(ui->camera_outer_right_name->currentIndex())
            .toStdString();
    Settings::values.camera_config[OuterRightCamera] =
        ui->camera_outer_right_config->text().toStdString();
    // outer left camera
    Settings::values.camera_name[OuterLeftCamera] =
        ui->camera_outer_left_name->itemText(ui->camera_outer_left_name->currentIndex())
            .toStdString();
    Settings::values.camera_config[OuterLeftCamera] =
        ui->camera_outer_left_config->text().toStdString();
    // inner camera
    Settings::values.camera_name[InnerCamera] =
        ui->camera_inner_name->itemText(ui->camera_inner_name->currentIndex()).toStdString();
    Settings::values.camera_config[InnerCamera] = ui->camera_inner_config->text().toStdString();
    Settings::Apply();
}
