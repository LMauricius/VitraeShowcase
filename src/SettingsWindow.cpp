#include "SettingsWindow.h"
#include "Vitrae/Collections/MethodCollection.hpp"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

#include <mutex>

SettingsWindow::SettingsWindow(AssetCollection &assetCollection, Status &status)
    : QMainWindow(), ui(), m_assetCollection(assetCollection), m_status(status), inputSpecshash(0)
{
    ui.setupUi(this);

    updateValues();

    connect(ui.light_dir_x, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double d) {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        this->m_assetCollection.p_scene->light.direction.x = d;
    });
    connect(ui.light_dir_y, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double d) {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        this->m_assetCollection.p_scene->light.direction.y = d;
    });
    connect(ui.light_dir_z, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double d) {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        this->m_assetCollection.p_scene->light.direction.z = d;
    });
    connect(ui.light_color, &QPushButton::clicked, [this]() {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        QColor lightColor = QColor::fromRgbF(m_assetCollection.p_scene->light.color_primary.r,
                                             m_assetCollection.p_scene->light.color_primary.g,
                                             m_assetCollection.p_scene->light.color_primary.b);
        QColor c = QColorDialog::getColor(lightColor, this);
        m_assetCollection.p_scene->light.color_primary = {c.redF(), c.greenF(), c.blueF()};
    });
    connect(ui.ambient_color, &QPushButton::clicked, [this]() {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        QColor lightColor = QColor::fromRgbF(m_assetCollection.p_scene->light.color_ambient.r,
                                             m_assetCollection.p_scene->light.color_ambient.g,
                                             m_assetCollection.p_scene->light.color_ambient.b);
        QColor c = QColorDialog::getColor(lightColor, this);
        m_assetCollection.p_scene->light.color_ambient = {c.redF(), c.greenF(), c.blueF()};
    });
    connect(ui.rebuildButton, &QPushButton::clicked, [this]() {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        m_assetCollection.comp.rebuildPipeline();
        ;
    });
    /*connect(ui.shadowMapSize, &QComboBox::currentTextChanged, [this](const QString &str) {
        this->m_assetCollection.comp.parameters.set("ShadowMapSize",
                                                    glm::vec2{str.toInt(), str.toInt()});
        this->m_assetCollection.shouldReloadPipelines = true;
    });
    m_assetCollection.comp.parameters.set("ShadowMapSize",
                                          glm::vec2{ui.shadowMapSize->currentText().toInt(),
                                                    ui.shadowMapSize->currentText().toInt()});*/

    MethodCollection &methodCollection = assetCollection.root.getComponent<MethodCollection>();

    // List outputs
    bool isFirst = true;
    for (auto outputName : methodCollection.getCompositorOutputs())
    {
        auto p_checkbox = new QCheckBox(ui.compositor_outputs_group);

        if (isFirst)
        {
            p_checkbox->setChecked(true);
            m_desiredOutputs.insert_back(ParamSpec{
                .name = outputName,
                .typeInfo = TYPE_INFO<void>,
            });
            isFirst = false;
        }
        else
        {
            p_checkbox->setChecked(false);
        }

        connect(p_checkbox, QOverload<int>::of(&QCheckBox::stateChanged),
                [this, outputName](int state)
                {
                    if (state == Qt::Checked)
                    {
                        this->m_desiredOutputs.insert_back(ParamSpec{
                            .name = outputName,
                            .typeInfo = TYPE_INFO<void>,
                        });
                    }
                    else
                    {
                        this->m_desiredOutputs.erase(outputName);
                    }
                    this->applyCompositorSettings();
                });

        ui.compositor_outputs_layout->addRow(QString::fromStdString(outputName), p_checkbox);
    }

    // list methods
    for (auto [target, options] : methodCollection.getPropertyOptionsMap())
    {
        auto p_combobox = new QComboBox(ui.shading_methods_group);

        for (auto &option : options)
        {
            p_combobox->addItem(
                QString::fromStdString(option));
        }

        p_combobox->setCurrentIndex(0);
        m_toBeAliases[target] = options[0];

        connect(p_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, target, p_combobox](int index)
                {
                    this->m_toBeAliases[target] = p_combobox->itemText(index).toStdString();
                    this->applyCompositorSettings();
                });

        ui.shading_methods_layout->addRow(QString::fromStdString(target), p_combobox);
    }

    connect(ui.rebuildButton, &QPushButton::clicked, [this]() {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        this->m_assetCollection.shouldReloadPipelines = true;
    });

    applyCompositorSettings();

    // list settings
    relistSettings();
}

SettingsWindow::~SettingsWindow() {}

void SettingsWindow::updateValues()
{
    ui.totalAvgDuraion->setText(QString::number(m_status.totalAvgFrameDuration.count() * 1000.0) +
                                "ms");
    ui.totalFPS->setText(QString::number(m_status.totalFPS));
    ui.currentAvgDuration->setText(
        QString::number(m_status.currentAvgFrameDuration.count() * 1000.0) + "ms");
    ui.currentFPS->setText(QString::number(m_status.currentFPS));

    // update spinboxes and other controls
    if (ui.camera_x->value() != m_assetCollection.p_scene->camera.position.x) {
        ui.camera_x->setValue(m_assetCollection.p_scene->camera.position.x);
    }
    if (ui.camera_y->value() != m_assetCollection.p_scene->camera.position.y) {
        ui.camera_y->setValue(m_assetCollection.p_scene->camera.position.y);
    }
    if (ui.camera_z->value() != m_assetCollection.p_scene->camera.position.z) {
        ui.camera_z->setValue(m_assetCollection.p_scene->camera.position.z);
    }
    auto cameraDirVector = m_assetCollection.p_scene->camera.rotation * glm::vec3(0, 0, 1);
    ui.camera_dir_x->setText(QString::number(cameraDirVector.x));
    ui.camera_dir_y->setText(QString::number(cameraDirVector.y));
    ui.camera_dir_z->setText(QString::number(cameraDirVector.z));
    if (ui.light_dir_x->value() != m_assetCollection.p_scene->light.direction.x) {
        ui.light_dir_x->setValue(m_assetCollection.p_scene->light.direction.x);
    }
    if (ui.light_dir_y->value() != m_assetCollection.p_scene->light.direction.y) {
        ui.light_dir_y->setValue(m_assetCollection.p_scene->light.direction.y);
    }
    if (ui.light_dir_z->value() != m_assetCollection.p_scene->light.direction.z) {
        ui.light_dir_z->setValue(m_assetCollection.p_scene->light.direction.z);
    }
    QColor lightColor(m_assetCollection.p_scene->light.color_primary.r * 255.0,
                      m_assetCollection.p_scene->light.color_primary.g * 255.0,
                      m_assetCollection.p_scene->light.color_primary.b * 255.0);
    ui.light_color->setPalette(
        QPalette(Qt::black, lightColor, lightColor, lightColor, lightColor, Qt::black, lightColor));
    lightColor = QColor(m_assetCollection.p_scene->light.color_ambient.r * 255.0,
                        m_assetCollection.p_scene->light.color_ambient.g * 255.0,
                        m_assetCollection.p_scene->light.color_ambient.b * 255.0);
    ui.ambient_color->setPalette(
        QPalette(Qt::black, lightColor, lightColor, lightColor, lightColor, Qt::black, lightColor));

    relistSettings();
}

void SettingsWindow::relistSettings()
{
    if (m_assetCollection.compositorInputsHash != inputSpecshash) {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);

        inputSpecshash = m_assetCollection.comp.getInputSpecs().getHash();

        // remove old inputs
        while (ui.settings_layout->count() > 0) {
            ui.settings_layout->removeRow(0);
        }

        // add new inputs
        for (auto &spec : m_assetCollection.comp.getInputSpecs().getSpecList()) {
            if (spec.typeInfo == TYPE_INFO<float>) {
                auto p_spinbox = new QDoubleSpinBox(ui.settings_group);
                p_spinbox->setSingleStep(0.1);
                p_spinbox->setMinimum(std::numeric_limits<float>::lowest());
                p_spinbox->setMaximum(std::numeric_limits<float>::max());
                if (auto vp = m_assetCollection.comp.parameters.getPtr(spec.name); vp) {
                    p_spinbox->setValue(vp->get<float>());
                }

                connect(
                    p_spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    [&m_assetCollection = this->m_assetCollection, name = spec.name](double val) {
                        m_assetCollection.comp.parameters.set(name, (float)val);
                    });

                ui.settings_layout->addRow(QString::fromStdString(spec.name), p_spinbox);
            } else if (spec.typeInfo == TYPE_INFO<double>) {
                auto p_spinbox = new QDoubleSpinBox(ui.settings_group);
                p_spinbox->setSingleStep(0.1);
                p_spinbox->setMinimum(std::numeric_limits<double>::lowest());
                p_spinbox->setMaximum(std::numeric_limits<double>::max());
                if (auto vp = m_assetCollection.comp.parameters.getPtr(spec.name); vp) {
                    p_spinbox->setValue(vp->get<double>());
                }

                connect(
                    p_spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    [&m_assetCollection = this->m_assetCollection, name = spec.name](double val) {
                        m_assetCollection.comp.parameters.set(name, (double)val);
                    });

                ui.settings_layout->addRow(QString::fromStdString(spec.name), p_spinbox);
            } else if (spec.typeInfo == TYPE_INFO<bool>) {
                auto p_checkbox = new QCheckBox(ui.settings_group);
                if (auto vp = m_assetCollection.comp.parameters.getPtr(spec.name); vp) {
                    p_checkbox->setChecked(vp->get<bool>());
                }
                connect(p_checkbox, &QCheckBox::toggled,
                        [&m_assetCollection = this->m_assetCollection, name = spec.name](bool val) {
                            m_assetCollection.comp.parameters.set(name, val);
                        });

                ui.settings_layout->addRow(QString::fromStdString(spec.name), p_checkbox);
            } else if (spec.typeInfo == TYPE_INFO<std::int32_t>) {
                std::int32_t def = 1;
                if (auto vp = m_assetCollection.comp.parameters.getPtr(spec.name); vp) {
                    def = vp->get<std::int32_t>();
                }
                m_assetCollection.comp.parameters.set(spec.name, def);

                auto p_spinbox = new QSpinBox(ui.settings_group);
                p_spinbox->setSingleStep(1);
                p_spinbox->setMinimum(std::numeric_limits<int>::lowest());
                p_spinbox->setMaximum(std::numeric_limits<int>::max());
                p_spinbox->setValue(def);

                connect(p_spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
                        [&m_assetCollection = this->m_assetCollection, name = spec.name](int val) {
                            m_assetCollection.comp.parameters.set(name, (std::int32_t)val);
                        });

                ui.settings_layout->addRow(QString::fromStdString(spec.name), p_spinbox);
            } else if (spec.typeInfo == TYPE_INFO<std::uint32_t>) {
                std::uint32_t def = 1;
                if (auto vp = m_assetCollection.comp.parameters.getPtr(spec.name); vp) {
                    def = vp->get<std::int32_t>();
                }
                m_assetCollection.comp.parameters.set(spec.name, def);

                if ((def & (def - 1)) == 0) { // if power of two
                    auto p_combobox = new QComboBox(ui.settings_group);
                    for (std::uint32_t i = std::max(2u, def / 256); i <= def * 256; i *= 2) {
                        p_combobox->addItem(QString::number(i));
                    }
                    p_combobox->setEditable(true);
                    p_combobox->setCurrentText(QString::number(def));

                    connect(p_combobox, &QComboBox::currentTextChanged,
                            [this, name = spec.name](const QString &str) {
                                this->m_assetCollection.comp.parameters.set(
                                    name, (std::uint32_t)str.toUInt());
                            });

                    ui.settings_layout->addRow(QString::fromStdString(spec.name), p_combobox);
                } else {
                    auto p_spinbox = new QSpinBox(ui.settings_group);
                    p_spinbox->setSingleStep(1);
                    p_spinbox->setMinimum(0);
                    p_spinbox->setMaximum(std::numeric_limits<int>::max());
                    p_spinbox->setValue(def);
                    connect(
                        p_spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
                        [&m_assetCollection = this->m_assetCollection, name = spec.name](int val) {
                            m_assetCollection.comp.parameters.set(name, (std::uint32_t)val);
                        });

                    ui.settings_layout->addRow(QString::fromStdString(spec.name), p_spinbox);
                }
            } else if (spec.typeInfo == TYPE_INFO<std::size_t>) {
                std::size_t def = 1;
                if (auto vp = m_assetCollection.comp.parameters.getPtr(spec.name); vp) {
                    def = vp->get<std::size_t>();
                }
                m_assetCollection.comp.parameters.set(spec.name, def);

                if ((def & (def - 1)) == 0) { // if power of two
                    auto p_combobox = new QComboBox(ui.settings_group);
                    for (std::size_t i = std::max((std::size_t)2, def / 256); i <= def * 256;
                         i *= 2) {
                        p_combobox->addItem(QString::number(i));
                    }
                    p_combobox->setEditable(true);
                    p_combobox->setCurrentText(QString::number(def));

                    connect(p_combobox, &QComboBox::currentTextChanged,
                            [this, name = spec.name](const QString &str) {
                                this->m_assetCollection.comp.parameters.set(
                                    name, (std::size_t)str.toUInt());
                            });

                    ui.settings_layout->addRow(QString::fromStdString(spec.name), p_combobox);
                } else {
                    auto p_spinbox = new QSpinBox(ui.settings_group);
                    p_spinbox->setSingleStep(1);
                    p_spinbox->setMinimum(0);
                    p_spinbox->setMaximum(std::numeric_limits<int>::max());
                    p_spinbox->setValue(def);
                    connect(
                        p_spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
                        [&m_assetCollection = this->m_assetCollection, name = spec.name](int val) {
                            m_assetCollection.comp.parameters.set(name, (std::size_t)val);
                        });

                    ui.settings_layout->addRow(QString::fromStdString(spec.name), p_spinbox);
                }
            } else if (spec.typeInfo == TYPE_INFO<glm::uvec2>) {
                glm::uvec2 def = {1, 1};
                if (auto vp = m_assetCollection.comp.parameters.getPtr(spec.name); vp) {
                    def = vp->get<glm::uvec2>();
                }
                m_assetCollection.comp.parameters.set(spec.name, def);

                if ((def.x & (def.x - 1)) == 0 && (def.y & (def.y - 1)) == 0) { // if power of two
                    auto p_combobox0 = new QComboBox(ui.settings_group);
                    auto p_combobox1 = new QComboBox(ui.settings_group);
                    for (std::uint32_t i = std::max(2u, def.x / 256); i <= def.x * 256; i *= 2) {
                        p_combobox0->addItem(QString::number(i));
                        p_combobox1->addItem(QString::number(i));
                    }
                    p_combobox0->setEditable(true);
                    p_combobox1->setEditable(true);
                    p_combobox0->setCurrentText(QString::number(def.x));
                    p_combobox1->setCurrentText(QString::number(def.y));

                    auto callback = [this, name = spec.name, p_combobox0,
                                     p_combobox1](const QString &str) {
                        this->m_assetCollection.comp.parameters.set(
                            name, glm::uvec2((std::uint32_t)p_combobox0->currentText().toUInt(),
                                             (std::uint32_t)p_combobox1->currentText().toUInt()));
                    };

                    connect(p_combobox0, &QComboBox::currentTextChanged, p_combobox1, callback);
                    connect(p_combobox1, &QComboBox::currentTextChanged, p_combobox0, callback);

                    QHBoxLayout *layout = new QHBoxLayout();
                    layout->addWidget(p_combobox0);
                    layout->addWidget(p_combobox1);
                    ui.settings_layout->addRow(QString::fromStdString(spec.name), layout);
                } else {
                    auto p_spinbox0 = new QSpinBox(ui.settings_group);
                    auto p_spinbox1 = new QSpinBox(ui.settings_group);
                    p_spinbox0->setSingleStep(1);
                    p_spinbox0->setMinimum(0);
                    p_spinbox0->setMaximum(std::numeric_limits<int>::max());
                    p_spinbox0->setValue(def.x);
                    p_spinbox1->setSingleStep(1);
                    p_spinbox1->setMinimum(0);
                    p_spinbox1->setMaximum(std::numeric_limits<int>::max());
                    p_spinbox1->setValue(def.y);
                    auto callback = [this, name = spec.name, p_spinbox0, p_spinbox1](int val) {
                        this->m_assetCollection.comp.parameters.set(
                            name, glm::uvec2((std::uint32_t)p_spinbox0->value(),
                                             (std::uint32_t)p_spinbox1->value()));
                    };

                    connect(p_spinbox0, QOverload<int>::of(&QSpinBox::valueChanged), p_spinbox1,
                            callback);
                    connect(p_spinbox1, QOverload<int>::of(&QSpinBox::valueChanged), p_spinbox0,
                            callback);
                    QHBoxLayout *layout = new QHBoxLayout();
                    layout->addWidget(p_spinbox0);
                    layout->addWidget(p_spinbox1);
                    ui.settings_layout->addRow(QString::fromStdString(spec.name), layout);
                }
            } else {
                // ignore
            }
        }

        // rebuild the pipeline
        m_assetCollection.shouldReloadPipelines = true;
    }
}

void SettingsWindow::applyCompositorSettings()
{
    std::unique_lock lock1(m_assetCollection.accessMutex);

    m_assetCollection.shouldReloadPipelines = true;

    // Aliases
    ParamAliases aliases(m_toBeAliases);
    m_assetCollection.comp.setParamAliases(aliases);

    // Outputs
    m_assetCollection.comp.setDesiredProperties(m_desiredOutputs);
}
