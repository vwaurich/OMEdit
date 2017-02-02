/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Open Source Modelica Consortium (OSMC),
 * c/o Linköpings universitet, Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 LICENSE OR
 * THIS OSMC PUBLIC LICENSE (OSMC-PL) VERSION 1.2.
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES RECIPIENT'S ACCEPTANCE
 * OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3, ACCORDING TO RECIPIENTS CHOICE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from OSMC, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or
 * http://www.openmodelica.org, and in the OpenModelica distribution.
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */
/*
 * @author Volker Waurich <volker.waurich@tu-dresden.de>
 */

#include <osg/MatrixTransform>
#include <osg/Vec3>
#include <osgDB/ReadFile>

#include "AbstractAnimationWindow.h"
#include "Modeling/MessagesWidget.h"
#include "Options/OptionsDialog.h"
#include "Modeling/MessagesWidget.h"
#include "Plotting/PlotWindowContainer.h"
#include "ViewerWidget.h"
#include "Visualizer.h"
#include "VisualizerMAT.h"
#include "VisualizerCSV.h"

/*!
 * \class AbstractAnimationWindow
 * \brief Abstract animation class defines a QMainWindow for animation.
 */
/*!
 * \brief AbstractAnimationWindow::AbstractAnimationWindow
 * \param pParent
 */
AbstractAnimationWindow::AbstractAnimationWindow(QWidget *pParent)
  : QMainWindow(pParent),
    mPathName(""),
    mFileName(""),
    mpVisualizer(nullptr),
    mpViewerWidget(nullptr),
    mpAnimationToolBar(new QToolBar(QString("Animation Toolbar"),this)),
    mpAnimationChooseFileAction(nullptr),
    mpAnimationInitializeAction(nullptr),
    mpAnimationPlayAction(nullptr),
    mpAnimationPauseAction(nullptr),
    mpAnimationSlider(nullptr),
    mpAnimationTimeLabel(nullptr),
    mpTimeTextBox(nullptr),
    mpAnimationSpeedLabel(nullptr),
    mpSpeedComboBox(nullptr),
    mpPerspectiveDropDownBox(nullptr),
    mpRotateCameraLeftAction(nullptr),
    mpRotateCameraRightAction(nullptr)
{
  // to distinguish this widget as a subwindow among the plotwindows
  setObjectName(QString("animationWidget"));
  //the viewer widget
  mpViewerWidget = new ViewerWidget(this);
  // we need to set the minimum height so that visualization window is still shown when we cascade windows.
  mpViewerWidget->setMinimumHeight(100);
  // Ensures that the signal customContextMenuRequested() to open the conext menu is emitted.
  mpViewerWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(mpViewerWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showShapePickContextMenu(const QPoint&)));
  // toolbar icon size
  int toolbarIconSize = OptionsDialog::instance()->getGeneralSettingsPage()->getToolbarIconSizeSpinBox()->value();
  mpAnimationToolBar->setIconSize(QSize(toolbarIconSize, toolbarIconSize));
  addToolBar(Qt::TopToolBarArea, mpAnimationToolBar);
  // Viewer layout
  QGridLayout *pGridLayout = new QGridLayout;
  pGridLayout->setContentsMargins(0, 0, 0, 0);
  pGridLayout->addWidget(mpViewerWidget);
  // add the viewer to the frame for boxed rectangle around it.
  QFrame *pCentralWidgetFrame = new QFrame;
  pCentralWidgetFrame->setFrameStyle(QFrame::StyledPanel);
  pCentralWidgetFrame->setLayout(pGridLayout);
  setCentralWidget(pCentralWidgetFrame);
}

/*!
 * \brief AbstractAnimationWindow::showShapePickContextMenu
 * \param pos
 */
void AbstractAnimationWindow::showShapePickContextMenu(const QPoint& pos)
{
  QString name = QString::fromStdString(mpViewerWidget->getSelectedShape());
  //std::cout<<"SHOW CONTEXT "<<name.toStdString()<<" compare "<<QString::compare(name,QString(""))<< std::endl;
  //the context widget
  QMenu contextMenu(tr("Context menu"), this);
  QMenu shapeMenu(name, this);
  QAction action1("Change Transparency", this);
  QAction action2("Reset All Shapes", this);
  //if a shape is picked, we can set it transparent
  if (0 != QString::compare(name,QString("")))
  {
    contextMenu.addMenu(&shapeMenu);
    shapeMenu.addAction(&action1);
    connect(&action1, SIGNAL(triggered()), this, SLOT(changeShapeTransparency()));
  }
  contextMenu.addAction(&action2);
  connect(&action2, SIGNAL(triggered()), this, SLOT(removeTransparencyForAllShapes()));
  connect(&contextMenu, SIGNAL(aboutToHide()), this, SLOT(doSomething()));
  contextMenu.exec(QWidget::mapToGlobal(pos));
}

/*!
 * \brief AbstractAnimationWindow::changeShapeTransparency
 * changes the transparency selection of a shape
 */
void AbstractAnimationWindow::changeShapeTransparency()
{
    ShapeObject* shape = nullptr;
    if (mpVisualizer->getBaseData()->getShapeObjectByID(mpViewerWidget->getSelectedShape(),&shape))
    {
      if (shape->_type.compare("dxf") == 0)
        std::cout<<"Transparency feature for DXF-Files is not applicable."<<std::endl;
      else
      {
        shape->setTransparency(not shape->getTransparency());
        mpVisualizer->updateVisAttributes(mpVisualizer->getTimeManager()->getVisTime());
        updateScene();
        mpViewerWidget->setSelectedShape("");
      }
    }
}

/*!
 * \brief AbstractAnimationWindow::removeTransparancyForAllShapes
 * sets all transparency settings back to default (nothing is transparent)
 *
 */
void AbstractAnimationWindow::removeTransparencyForAllShapes()
{
  if (mpVisualizer != NULL)
  {
    std::vector<ShapeObject>* shapes = nullptr;
    shapes = &mpVisualizer->getBaseData()->_shapes;
    for(std::vector<ShapeObject>::iterator shape = shapes->begin() ; shape < shapes->end(); ++shape )
    {
    shape->setTransparency(false);
    }
    mpVisualizer->updateVisAttributes(mpVisualizer->getTimeManager()->getVisTime());
    updateScene();
  }
}

/*!
 * \brief AbstractAnimationWindow::openAnimationFile
 * \param fileName
 */
void AbstractAnimationWindow::openAnimationFile(QString fileName)
{
  std::string file = fileName.toStdString();
  if (file.compare("")) {
    std::size_t pos = file.find_last_of("/\\");
    mPathName = file.substr(0, pos + 1);
    mFileName = file.substr(pos + 1, file.length());
    //std::cout<<"file "<<mFileName<<"   path "<<mPathName<<std::endl;
    if (loadVisualization()) {
      // start the widgets
      mpAnimationInitializeAction->setEnabled(true);
      mpAnimationPlayAction->setEnabled(true);
      mpAnimationPauseAction->setEnabled(true);
      mpAnimationSlider->setEnabled(true);
      bool state = mpAnimationSlider->blockSignals(true);
      mpAnimationSlider->setValue(0);
      mpAnimationSlider->blockSignals(state);
      mpSpeedComboBox->setEnabled(true);
      mpTimeTextBox->setEnabled(true);
      mpTimeTextBox->setText(QString::number(mpVisualizer->getTimeManager()->getStartTime()));
      /* Only use isometric view as default for csv file type.
       * Otherwise use side view as default which suits better for Modelica models.
       */
      if (isCSV(mFileName)) {
        mpPerspectiveDropDownBox->setCurrentIndex(0);
        cameraPositionIsometric();
      } else {
        mpPerspectiveDropDownBox->setCurrentIndex(1);
        cameraPositionSide();
      }
    }
  }
}

void AbstractAnimationWindow::createActions()
{
  // actions and widgets for the toolbar
  int toolbarIconSize = OptionsDialog::instance()->getGeneralSettingsPage()->getToolbarIconSizeSpinBox()->value();
  // choose file action
  mpAnimationChooseFileAction = new QAction(QIcon(":/Resources/icons/open.svg"), Helper::animationChooseFile, this);
  mpAnimationChooseFileAction->setStatusTip(Helper::animationChooseFileTip);
  connect(mpAnimationChooseFileAction, SIGNAL(triggered()),this, SLOT(chooseAnimationFileSlotFunction()));
  // initialize action
  mpAnimationInitializeAction = new QAction(QIcon(":/Resources/icons/initialize.svg"), Helper::animationInitialize, this);
  mpAnimationInitializeAction->setStatusTip(Helper::animationInitializeTip);
  mpAnimationInitializeAction->setEnabled(false);
  connect(mpAnimationInitializeAction, SIGNAL(triggered()),this, SLOT(initSlotFunction()));
  // animation play action
  mpAnimationPlayAction = new QAction(QIcon(":/Resources/icons/play_animation.svg"), Helper::animationPlay, this);
  mpAnimationPlayAction->setStatusTip(Helper::animationPlayTip);
  mpAnimationPlayAction->setEnabled(false);
  connect(mpAnimationPlayAction, SIGNAL(triggered()),this, SLOT(playSlotFunction()));
  // animation pause action
  mpAnimationPauseAction = new QAction(QIcon(":/Resources/icons/pause.svg"), Helper::animationPause, this);
  mpAnimationPauseAction->setStatusTip(Helper::animationPauseTip);
  mpAnimationPauseAction->setEnabled(false);
  connect(mpAnimationPauseAction, SIGNAL(triggered()),this, SLOT(pauseSlotFunction()));
  // animation slide
  mpAnimationSlider = new QSlider(Qt::Horizontal);
  mpAnimationSlider->setMinimum(0);
  mpAnimationSlider->setMaximum(100);
  mpAnimationSlider->setSliderPosition(0);
  mpAnimationSlider->setEnabled(false);
  connect(mpAnimationSlider, SIGNAL(valueChanged(int)),this, SLOT(sliderSetTimeSlotFunction(int)));
  // animation time
  QDoubleValidator *pDoubleValidator = new QDoubleValidator(this);
  pDoubleValidator->setBottom(0);
  mpAnimationTimeLabel = new Label;
  mpAnimationTimeLabel->setText(tr("Time [s]:"));
  mpTimeTextBox = new QLineEdit("0.0");
  mpTimeTextBox->setMaximumSize(QSize(toolbarIconSize*2, toolbarIconSize));
  mpTimeTextBox->setEnabled(false);
  mpTimeTextBox->setValidator(pDoubleValidator);
  connect(mpTimeTextBox, SIGNAL(returnPressed()),this, SLOT(jumpToTimeSlotFunction()));
  // animation speed
  mpAnimationSpeedLabel = new Label;
  mpAnimationSpeedLabel->setText(tr("Speed:"));
  mpSpeedComboBox = new QComboBox;
  mpSpeedComboBox->setEditable(true);
  mpSpeedComboBox->addItems(QStringList() << "10" << "5" << "2" << "1" << "0.5" << "0.2" << "0.1");
  mpSpeedComboBox->setCurrentIndex(3);
  mpSpeedComboBox->setMaximumSize(QSize(toolbarIconSize*2, toolbarIconSize));
  mpSpeedComboBox->setEnabled(false);
  mpSpeedComboBox->setValidator(pDoubleValidator);
  mpSpeedComboBox->setCompleter(0);
  connect(mpSpeedComboBox, SIGNAL(currentIndexChanged(int)),this, SLOT(setSpeedSlotFunction()));
  connect(mpSpeedComboBox->lineEdit(), SIGNAL(textChanged(QString)),this, SLOT(setSpeedSlotFunction()));
  // perspective drop down
  mpPerspectiveDropDownBox = new QComboBox;
  mpPerspectiveDropDownBox->addItem(QIcon(":/Resources/icons/perspective0.svg"), QString("Isometric"));
  mpPerspectiveDropDownBox->addItem(QIcon(":/Resources/icons/perspective1.svg"),QString("Side"));
  mpPerspectiveDropDownBox->addItem(QIcon(":/Resources/icons/perspective2.svg"),QString("Front"));
  mpPerspectiveDropDownBox->addItem(QIcon(":/Resources/icons/perspective3.svg"),QString("Top"));
  connect(mpPerspectiveDropDownBox, SIGNAL(activated(int)), this, SLOT(setPerspective(int)));
  // rotate camera left action
  mpRotateCameraLeftAction = new QAction(QIcon(":/Resources/icons/rotateCameraLeft.svg"), tr("Rotate Left"), this);
  mpRotateCameraLeftAction->setStatusTip(tr("Rotates the camera left"));
  connect(mpRotateCameraLeftAction, SIGNAL(triggered()), this, SLOT(rotateCameraLeft()));
  // rotate camera right action
  mpRotateCameraRightAction = new QAction(QIcon(":/Resources/icons/rotateCameraRight.svg"), tr("Rotate Right"), this);
  mpRotateCameraRightAction->setStatusTip(tr("Rotates the camera right"));
  connect(mpRotateCameraRightAction, SIGNAL(triggered()), this, SLOT(rotateCameraRight()));
}

/*!
 * \brief AbstractAnimationWindow::clearView
 */
void AbstractAnimationWindow::clearView()
{
  if (mpViewerWidget) {
    mpViewerWidget->getSceneView()->setSceneData(0);
    mpViewerWidget->update();
  }
}

/*!
 * \brief AbstractAnimationWindow::loadVisualization
 * loads the data and the xml scene description
 * \return
 */
bool AbstractAnimationWindow::loadVisualization()
{
  VisType visType = VisType::NONE;
  // Get visualization type.
  if (isFMU(mFileName)) {
    visType = VisType::FMU;
  } else if (isMAT(mFileName)) {
    visType = VisType::MAT;
  } else if (isCSV(mFileName)) {
    visType = VisType::CSV;
  } else {
    MessagesWidget::instance()->addGUIMessage(MessageItem(MessageItem::Modelica, "", false, 0, 0, 0, 0, tr("Unknown visualization type."),
                                                          Helper::scriptingKind, Helper::errorLevel));
    return false;
  }
  //load the XML File, build osgTree, get initial values for the shapes
  bool xmlExists = checkForXMLFile(mFileName, mPathName);
  if (!xmlExists) {
    QString msg = tr("Could not find the visual XML file %1.").arg(QString(assembleXMLFileName(mFileName, mPathName).c_str()));
    MessagesWidget::instance()->addGUIMessage(MessageItem(MessageItem::Modelica, "", false, 0, 0, 0, 0, msg, Helper::scriptingKind,
                                                          Helper::errorLevel));
    return false;
  } else {
    //init visualizer
    if (visType == VisType::MAT) {
      mpVisualizer = new VisualizerMAT(mFileName, mPathName);
    } else if (visType == VisType::CSV) {
      mpVisualizer = new VisualizerCSV(mFileName, mPathName);
    } else if (visType == VisType::FMU) {
      mpVisualizer = new VisualizerFMU(mFileName, mPathName);
    } else {
      QString msg = tr("Could not init %1 %2.").arg(QString(mPathName.c_str())).arg(QString(mFileName.c_str()));
      MessagesWidget::instance()->addGUIMessage(MessageItem(MessageItem::Modelica, "", false, 0, 0, 0, 0, msg, Helper::scriptingKind,
                                                            Helper::errorLevel));
      return false;
    }
    connect(mpVisualizer->getTimeManager()->getUpdateSceneTimer(), SIGNAL(timeout()), SLOT(updateScene()));
    mpVisualizer->initData();
    mpVisualizer->setUpScene();
    mpVisualizer->initVisualization();
    //add scene for the chosen visualization
    mpViewerWidget->getSceneView()->setSceneData(mpVisualizer->getOMVisScene()->getScene().getRootNode());
  }
  //add window title
  setWindowTitle(QString::fromStdString(mFileName));
  //open settings dialog for FMU simulation
  if (visType == VisType::FMU) {
    openFMUSettingsDialog(dynamic_cast<VisualizerFMU*>(mpVisualizer));
  }
  return true;
}

/*!
 * \brief AbstractAnimationWindow::resetCamera
 * resets the camera position
 */
void AbstractAnimationWindow::resetCamera()
{
  mpViewerWidget->getSceneView()->home();
  mpViewerWidget->update();
}

/*!
 * \brief AbstractAnimationWindow::cameraPositionIsometric
 * sets the camera position to isometric view
 */
void AbstractAnimationWindow::cameraPositionIsometric()
{
  double d = computeDistanceToOrigin();
  osg::Matrixd mat = osg::Matrixd(0.7071, 0, -0.7071, 0,
                                  -0.409, 0.816, -0.409, 0,
                                  0.57735,  0.57735, 0.57735, 0,
                                  0.57735*d, 0.57735*d, 0.57735*d, 1);
  mpViewerWidget->getSceneView()->getCameraManipulator()->setByMatrix(mat);
  mpViewerWidget->update();
}

/*!
 * \brief AbstractAnimationWindow::cameraPositionSide
 * sets the camera position to Side
 */
void AbstractAnimationWindow::cameraPositionSide()
{
  double d = computeDistanceToOrigin();
  osg::Matrixd mat = osg::Matrixd(1, 0, 0, 0,
                                  0, 1, 0, 0,
                                  0, 0, 1, 0,
                                  0, 0, d, 1);
  mpViewerWidget->getSceneView()->getCameraManipulator()->setByMatrix(mat);
  mpViewerWidget->update();
}

/*!
 * \brief AbstractAnimationWindow::cameraPositionFront
 * sets the camera position to Front
 */
void AbstractAnimationWindow::cameraPositionFront()
{
  double d = computeDistanceToOrigin();
  osg::Matrixd mat = osg::Matrixd(0, 0, 1, 0,
                                  1, 0, 0, 0,
                                  0, 1, 0, 0,
                                  0, d, 0, 1);
  mpViewerWidget->getSceneView()->getCameraManipulator()->setByMatrix(mat);
  mpViewerWidget->update();
}

/*!
 * \brief AbstractAnimationWindow::cameraPositionTop
 * sets the camera position to Top
 */
void AbstractAnimationWindow::cameraPositionTop()
{
  double d = computeDistanceToOrigin();
  osg::Matrixd mat = osg::Matrixd( 0, 0,-1, 0,
                                   0, 1, 0, 0,
                                   1, 0, 0, 0,
                                   d, 0, 0, 1);
  mpViewerWidget->getSceneView()->getCameraManipulator()->setByMatrix(mat);
  mpViewerWidget->update();
}

/*!
 * \brief AbstractAnimationWindow::computeDistanceToOrigin
 * computes distance to origin using pythagoras theorem
 */
double AbstractAnimationWindow::computeDistanceToOrigin()
{
  osg::ref_ptr<osgGA::CameraManipulator> manipulator = mpViewerWidget->getSceneView()->getCameraManipulator();
  osg::Matrixd mat = manipulator->getMatrix();
  //assemble

  //Compute distance to center using pythagoras theorem
  double d = sqrt(abs(mat(3,0))*abs(mat(3,0))+
                  abs(mat(3,1))*abs(mat(3,1))+
                  abs(mat(3,2))*abs(mat(3,2)));

  //If d is very small (~0), set it to 1 as default
  if(d < 1e-10) {
    d=1;
  }

  return d;
}

/*!
 * \brief AbstractAnimationWindow::openFMUSettingsDialog
 * Opens a dialog to set the settings for the FMU visualization
 * \param pVisualizerFMU
 */
void AbstractAnimationWindow::openFMUSettingsDialog(VisualizerFMU* pVisualizerFMU)
{
  FMUSettingsDialog *pFMUSettingsDialog = new FMUSettingsDialog(this, pVisualizerFMU);
  pFMUSettingsDialog->exec();
}

/*!
 * \brief AbstractAnimationWindow::updateSceneFunction
 * updates the visualization objects
 */
void AbstractAnimationWindow::updateScene()
{
  if (!(mpVisualizer == NULL)) {
    //set time label
    if (!mpVisualizer->getTimeManager()->isPaused()) {
      mpTimeTextBox->setText(QString::number(mpVisualizer->getTimeManager()->getVisTime()));
      // set time slider
      if (mpVisualizer->getVisType() != VisType::FMU) {
        int time = mpVisualizer->getTimeManager()->getTimeFraction();
        bool state = mpAnimationSlider->blockSignals(true);
        mpAnimationSlider->setValue(time);
        mpAnimationSlider->blockSignals(state);
      }
    }
    //update the scene
    mpVisualizer->sceneUpdate();
    mpViewerWidget->update();
  }
}

/*!
 * \brief AbstractAnimationWindow::animationFileSlotFunction
 * opens a file dialog to chooes an animation
 */
void AbstractAnimationWindow::chooseAnimationFileSlotFunction()
{
  QString fileName = StringHandler::getOpenFileName(this, QString("%1 - %2").arg(Helper::applicationName).arg(Helper::chooseFile),
                                                    NULL, Helper::visualizationFileTypes, NULL);
  if (fileName.isEmpty()) {
    return;
  }
  openAnimationFile(fileName);
}

/*!
 * \brief AbstractAnimationWindow::initSlotFunction
 * slot function for the init button
 */
void AbstractAnimationWindow::initSlotFunction()
{
  mpVisualizer->initVisualization();
  bool state = mpAnimationSlider->blockSignals(true);
  mpAnimationSlider->setValue(0);
  mpAnimationSlider->blockSignals(state);
  mpTimeTextBox->setText(QString::number(mpVisualizer->getTimeManager()->getVisTime()));
  mpViewerWidget->update();
}

/*!
 * \brief AbstractAnimationWindow::playSlotFunction
 * slot function for the play button
 */
void AbstractAnimationWindow::playSlotFunction()
{
  mpVisualizer->getTimeManager()->setPause(false);
}

/*!
 * \brief AbstractAnimationWindow::pauseSlotFunction
 * slot function for the pause button
 */
void AbstractAnimationWindow::pauseSlotFunction()
{
  mpVisualizer->getTimeManager()->setPause(true);
}

/*!
 * \brief AbstractAnimationWindow::sliderSetTimeSlotFunction
 * slot function for the time slider to jump to the adjusted point of time
 */
void AbstractAnimationWindow::sliderSetTimeSlotFunction(int value)
{
  float time = (mpVisualizer->getTimeManager()->getEndTime()
                - mpVisualizer->getTimeManager()->getStartTime())
      * (float) (value / 100.0);
  mpVisualizer->getTimeManager()->setVisTime(time);
  mpTimeTextBox->setText(QString::number(mpVisualizer->getTimeManager()->getVisTime()));
  mpVisualizer->updateScene(time);
  mpViewerWidget->update();
}

/*!
 * \brief AbstractAnimationWindow::jumpToTimeSlotFunction
 * slot function to jump to the user input point of time
 */
void AbstractAnimationWindow::jumpToTimeSlotFunction()
{
  QString str = mpTimeTextBox->text();
  bool isFloat = true;
  double start = mpVisualizer->getTimeManager()->getStartTime();
  double end = mpVisualizer->getTimeManager()->getEndTime();
  double value = str.toFloat(&isFloat);
  if (isFloat && value >= 0.0) {
    if (value < start) {
      value = start;
    } else if (value > end) {
      value = end;
    }
    mpVisualizer->getTimeManager()->setVisTime(value);
    bool state = mpAnimationSlider->blockSignals(true);
    mpAnimationSlider->setValue(mpVisualizer->getTimeManager()->getTimeFraction());
    mpAnimationSlider->blockSignals(state);
    mpVisualizer->updateScene(value);
    mpViewerWidget->update();
  }
}

/*!
 * \brief AbstractAnimationWindow::setSpeedUpSlotFunction
 * slot function to set the user input speed up
 */
void AbstractAnimationWindow::setSpeedSlotFunction()
{
  QString str = mpSpeedComboBox->lineEdit()->text();
  bool isFloat = true;
  double value = str.toFloat(&isFloat);
  if (isFloat && value > 0.0) {
    mpVisualizer->getTimeManager()->setSpeedUp(value);
    mpViewerWidget->update();
  }
}

/*!
 * \brief AbstractAnimationWindow::setPerspective
 * gets the identifier for the chosen perspective and calls the functions
 */
void AbstractAnimationWindow::setPerspective(int value)
{
  switch(value) {
    case 0:
      cameraPositionIsometric();
      break;
    case 1:
      cameraPositionSide();
      break;
    case 2:
      cameraPositionTop();
      break;
    case 3:
      cameraPositionFront();
      break;
  }
}

/*!
 * \brief AbstractAnimationWindow::rotateCameraLeft
 * rotates the camera 90 degress left about the line of sight
 */
void AbstractAnimationWindow::rotateCameraLeft()
{
  osg::ref_ptr<osgGA::CameraManipulator> manipulator = mpViewerWidget->getSceneView()->getCameraManipulator();
  osg::Matrixd mat = manipulator->getMatrix();
  osg::Camera *pCamera = mpViewerWidget->getSceneView()->getCamera();

  osg::Vec3d eye, center, up;
  pCamera->getViewMatrixAsLookAt(eye, center, up);
  osg::Vec3d rotationAxis = center-eye;

  osg::Matrixd rotMatrix;
  rotMatrix.makeRotate(3.1415/2.0, rotationAxis);

  mpViewerWidget->getSceneView()->getCameraManipulator()->setByMatrix(mat*rotMatrix);
  mpViewerWidget->update();
}

/*!
 * \brief AbstractAnimationWindow::rotateCameraRight
 * rotates the camera 90 degress right about the line of sight
 */
void AbstractAnimationWindow::rotateCameraRight()
{
  osg::ref_ptr<osgGA::CameraManipulator> manipulator = mpViewerWidget->getSceneView()->getCameraManipulator();
  osg::Matrixd mat = manipulator->getMatrix();
  osg::Camera *pCamera = mpViewerWidget->getSceneView()->getCamera();

  osg::Vec3d eye, center, up;
  pCamera->getViewMatrixAsLookAt(eye, center, up);
  osg::Vec3d rotationAxis = center-eye;

  osg::Matrixd rotMatrix;
  rotMatrix.makeRotate(-3.1415/2.0, rotationAxis);

  mpViewerWidget->getSceneView()->getCameraManipulator()->setByMatrix(mat*rotMatrix);
  mpViewerWidget->update();
}
