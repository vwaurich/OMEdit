/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-2014, Open Source Modelica Consortium (OSMC),
 * c/o Linköpings universitet, Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 LICENSE OR
 * THIS OSMC PUBLIC LICENSE (OSMC-PL) VERSION 1.2.
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES
 * RECIPIENT'S ACCEPTANCE OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3,
 * ACCORDING TO RECIPIENTS CHOICE.
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
 * @author Adeel Asghar <adeel.asghar@liu.se>
 */

#ifndef PLOTWINDOWCONTAINER_H
#define PLOTWINDOWCONTAINER_H

#if !defined(WITHOUT_OSG)
#include "Animation/AnimationWindow.h"
#endif

#include "Util/Utilities.h"
#include "OMPlot.h"

class AnimationWindow;

class PlotWindowContainer : public QMdiArea
{
  Q_OBJECT
public:
  PlotWindowContainer(QWidget *pParent = 0);
  QString getUniqueName(QString name = QString("Plot"), int number = 1);
  OMPlot::PlotWindow* getCurrentWindow();
  OMPlot::PlotWindow* getTopPlotWindow();
  void setTopPlotWindowActive();
#if !defined(WITHOUT_OSG)
  AnimationWindow* getCurrentAnimationWindow();
#endif
  bool eventFilter(QObject *pObject, QEvent *pEvent);
public slots:
  void addPlotWindow(bool maximized = false);
  void addParametricPlotWindow();
  void addArrayPlotWindow(bool maximized = false);
  void addArrayParametricPlotWindow();
  void addAnimationWindow(bool maximized = false);
  void clearPlotWindow();
  void exportVariables();
  void updatePlotWindows(QString variable);
};

#endif // PLOTWINDOWCONTAINER_H
