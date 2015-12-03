#pragma once

#include <QtGui>
#include <QWidget>
#include "../model/model.h"

class graph_widget : public QWidget
{
    Q_OBJECT
 public:

    graph_widget();

    void init(int graph_size, rk_real *data, int size);

 protected:
    void paintEvent(QPaintEvent *event);
 signals:

 public slots:

 private:
  rk_real *m_data;
  int m_size;
  int m_graph_size;

  rk_real m_scale,m_offset,m_min,m_max;
};
