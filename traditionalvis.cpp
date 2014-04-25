#include "traditionalvis.h"
#include <iostream>
#include <QFontMetrics>

TraditionalVis::TraditionalVis(QWidget * parent, VisOptions * _options)
    : TimelineVis(parent = parent, _options),
    minTime(0),
    maxTime(0),
    startTime(0),
    timeSpan(0),
    stepToTime(new QVector<TimePair *>()),
    lassoRect(QRect())
{

}

TraditionalVis::~TraditionalVis()
{
    for (QVector<TimePair *>::Iterator itr = stepToTime->begin();
         itr != stepToTime->end(); itr++)
    {
        delete *itr;
        *itr = NULL;
    }
    delete stepToTime;
}


void TraditionalVis::setTrace(Trace * t)
{
    VisWidget::setTrace(t);

    // Initial conditions
    startStep = 0;
    int stopStep = startStep + initStepSpan;
    startProcess = 0;
    processSpan = trace->num_processes;
    startPartition = 0;

    // Determine/save time information
    minTime = ULLONG_MAX;
    maxTime = 0;
    startTime = ULLONG_MAX;
    unsigned long long stopTime = 0;
    maxStep = trace->global_max_step;
    for (QList<Partition*>::Iterator part = trace->partitions->begin();
         part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list
             = (*part)->events->begin(); event_list != (*part)->events->end();
             ++event_list)
        {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                if ((*evt)->exit > maxTime)
                    maxTime = (*evt)->exit;
                if ((*evt)->enter < minTime)
                    minTime = (*evt)->enter;
                if ((*evt)->step >= boundStep(startStep)
                        && (*evt)->enter < startTime)
                    startTime = (*evt)->enter;
                if ((*evt)->step <= boundStep(stopStep)
                        && (*evt)->exit > stopTime)
                    stopTime = (*evt)->exit;
            }
        }
    }
    timeSpan = stopTime - startTime;
    stepSpan = stopStep - startStep;

    // Clean up old
    for (QVector<TimePair *>::Iterator itr = stepToTime->begin();
         itr != stepToTime->end(); itr++)
    {
        delete *itr;
        *itr = NULL;
    }
    delete stepToTime;

    // Create time/step mapping
    stepToTime = new QVector<TimePair *>();
    for (int i = 0; i < maxStep/2 + 1; i++)
        stepToTime->insert(i, new TimePair(ULLONG_MAX, 0));
    int step;
    for (QList<Partition*>::Iterator part = trace->partitions->begin();
         part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list
             = (*part)->events->begin(); event_list != (*part)->events->end();
             ++event_list)
        {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                if ((*evt)->step < 0)
                    continue;
                step = (*evt)->step / 2;
                if ((*stepToTime)[step]->start > (*evt)->enter)
                    (*stepToTime)[step]->start = (*evt)->enter;
                if ((*stepToTime)[step]->stop < (*evt)->exit)
                    (*stepToTime)[step]->stop = (*evt)->exit;
            }
        }
    }

}

void TraditionalVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    int x = event->x();
    int y = event->y();
    for (QMap<Event *, QRect>::Iterator evt = drawnEvents.begin();
         evt != drawnEvents.end(); ++evt)
        if (evt.value().contains(x,y))
        {
            if (evt.key() == selected_event)
            {
                selected_event = NULL;
            }
            else
            {
                selected_aggregate = false;
                selected_event = evt.key();
            }
            break;
        }

    changeSource = true;
    emit eventClicked(selected_event, false);
    repaint();
}

void TraditionalVis::rightDrag(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    mousex = event->x();
    mousey = event->y();
    if (pressx < event->x())
    {
        startTime = startTime + timeSpan * pressx / rect().width();
        timeSpan = timeSpan * (event->x() - pressx) / rect().width();
    }
    else
    {
        startTime = startTime + timeSpan * event->x() / rect().width();
        timeSpan = timeSpan * (pressx - event->x()) / rect().width();
    }
    if (startTime < minTime)
        startTime = minTime;
    if (startTime > maxTime)
        startTime = maxTime;

    if (pressy < event->y())
    {
        startProcess = startProcess + processSpan * pressy
                / (rect().height() - timescaleHeight);
        processSpan = processSpan * (event->y() - pressy)
                / (rect().height() - timescaleHeight);
    }
    else
    {
        startProcess = startProcess + processSpan * event->y()
                / (rect().height() - timescaleHeight);
        processSpan = processSpan * (pressy - event->y())
                / (rect().height() - timescaleHeight);
    }
    if (startProcess + processSpan > trace->num_processes)
        startProcess = trace->num_processes - processSpan;
    if (startProcess < 0)
        startProcess = 0;


    repaint();
    changeSource = true;
    emit stepsChanged(startStep, startStep + stepSpan, false);
}


void TraditionalVis::mouseMoveEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    lassoRect = QRect();
    if (mousePressed && !rightPressed) {
        lastStartStep = startStep;
        int diffx = mousex - event->x();
        int diffy = mousey - event->y();
        startTime += timeSpan / 1.0 / rect().width() * diffx;
        startProcess += diffy / 1.0 / processheight;

        if (startTime < minTime)
            startTime = minTime;
        if (startTime > maxTime)
            startTime = maxTime;

        if (startProcess + processSpan > trace->num_processes)
            startProcess = trace->num_processes - processSpan;
        if (startProcess < 0)
            startProcess = 0;


        mousex = event->x();
        mousey = event->y();
        repaint();
    }
    else if (mousePressed && rightPressed)
    {
        lassoRect = QRect(std::min(pressx, event->x()),
                          std::min(pressy, event->y()),
                          abs(pressx - event->x()),
                          abs(pressy - event->y()));
        repaint();
    }
    else // potential hover
    {
        mousex = event->x();
        mousey = event->y();
        if (hover_event == NULL
                || !drawnEvents[hover_event].contains(mousex, mousey))
        {
            hover_event = NULL;
            for (QMap<Event *, QRect>::Iterator evt = drawnEvents.begin();
                 evt != drawnEvents.end(); ++evt)
                if (evt.value().contains(mousex, mousey))
                    hover_event = evt.key();

            repaint();
        }
    }

    // startStep & stepSpan are calculated during painting when we
    // examine each element
    if (mousePressed) {
        changeSource = true;
        emit stepsChanged(startStep, startStep + stepSpan, false);
    }
}

void TraditionalVis::wheelEvent(QWheelEvent * event)
{
    if (!visProcessed)
        return;

    float scale = 1;
    int clicks = event->delta() / 8 / 15;
    scale = 1 + clicks * 0.05;
    if (Qt::MetaModifier && event->modifiers()) {
        // Vertical
        float avgProc = startProcess + processSpan / 2.0;
        processSpan *= scale;
        startProcess = avgProc - processSpan / 2.0;
    } else {
        // Horizontal
        lastStartStep = startStep;
        float middleTime = startTime + timeSpan / 2.0;
        timeSpan *= scale;
        startTime = middleTime - timeSpan / 2.0;
    }
    repaint();
    changeSource = true;
    // startStep & stepSpan are calculated during painting when we
    // examine each element
    emit stepsChanged(startStep, startStep + stepSpan, false);
}


void TraditionalVis::setSteps(float start, float stop, bool jump)
{
    if (!visProcessed)
        return;

    if (changeSource) {
        changeSource = false;
        return;
    }
    lastStartStep = startStep;
    startStep = start;
    stepSpan = stop - start + 1;
    startTime = (*stepToTime)[std::max(boundStep(start)/2, 0)]->start;
    timeSpan = (*stepToTime)[std::min(boundStep(stop)/2,  maxStep/2)]->stop
            - startTime;
    jumped = jump;

    if (!closed) {
        repaint();
    }
}

void TraditionalVis::prepaint()
{
    drawnEvents.clear();
    if (jumped) // We have to redo the active_partitions
    {
        // We know this list is in order, so we only have to go so far
        //int topStep = boundStep(startStep + stepSpan) + 1;
        int bottomStep = floor(startStep) - 1;
        Partition * part = NULL;
        for (int i = 0; i < trace->partitions->length(); ++i)
        {
            part = trace->partitions->at(i);
            if (part->max_global_step >= bottomStep)
            {
                startPartition = i;
                break;
            }
        }
    }
    else // We nudge the active partitions as necessary
    {
        int bottomStep = floor(startStep) - 1;
        Partition * part = NULL;
        if (startStep < lastStartStep) // check earlier partitions
        {
            // Keep setting the one before until its right
            for (int i = startPartition; i >= 0; --i)
            {
                part = trace->partitions->at(i);
                if (part->max_global_step >= bottomStep)
                {
                    startPartition = i;
                }
                else
                {
                    break;
                }
            }
        }
        else if (startStep > lastStartStep) // check current partitions up
        {
            // As soon as we find one, we're good
            for (int i = startPartition; i < trace->partitions->length(); ++i)
            {
                part = trace->partitions->at(i);
                if (part->max_global_step >= bottomStep)
                {
                    startPartition = i;
                    break;
                }
            }
        }
    }
}


void TraditionalVis::drawNativeGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!visProcessed)
        return;

    int effectiveHeight = rect().height() - timescaleHeight;
    if (effectiveHeight / processSpan >= 3 && rect().width() / stepSpan >= 3)
        return;

    QString metric(options->metric);
    unsigned long long stopTime = startTime + timeSpan;

    // Setup viewport
    int width = rect().width();
    int height = effectiveHeight;
    float effectiveSpan = timeSpan;

    glViewport(0,
               timescaleHeight,
               width,
               height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, effectiveSpan, 0, processSpan, 0, 1);

    float barheight = 1.0;
    processheight = height/ processSpan;

    // Generate buffers to hold each bar. We don't know how many there will
    // be since we draw one per event.
    QVector<GLfloat> bars = QVector<GLfloat>();
    QVector<GLfloat> colors = QVector<GLfloat>();

    // Process events for values
    float x, y, w; // true position
    float position; // placement of process
    Partition * part = NULL;
    int step, stopStep = 0;
    int upperStep = startStep + stepSpan + 2;
    startStep = maxStep;
    QColor color;
    float maxProcess = processSpan + startProcess;
    for (int i = startPartition; i < trace->partitions->length(); ++i)
    {
        part = trace->partitions->at(i);
        if (part->min_global_step > upperStep)
            break;

        for (QMap<int, QList<Event *> *>::Iterator event_list
             = part->events->begin(); event_list != part->events->end();
             ++event_list)
        {
            position = proc_to_order[event_list.key()];
             // Out of process span test
            if (position < floor(startProcess)
                    || position > ceil(startProcess + processSpan))
                continue;
            y = (maxProcess - position) * barheight - 1;
            for (QList<Event *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                // Out of time span test
                if ((*evt)->exit < startTime || (*evt)->enter > stopTime)
                    continue;


                // save step information for emitting
                step = (*evt)->step;
                if (step >= 0 && step > stopStep)
                    stopStep = step;
                if (step >= 0 && step < startStep) {
                    startStep = step;
                }

                // Calculate position of this bar in float space
                w = (*evt)->exit - (*evt)->enter;
                x = 0;
                if ((*evt)->enter >= startTime)
                    x = (*evt)->enter - startTime;
                else
                    w -= (startTime - (*evt)->enter);

                color = options->colormap->color((*(*evt)->metrics)[metric]->event);
                if (options->colorTraditionalByMetric
                        && (*evt)->hasMetric(options->metric))
                    color= options->colormap->color((*evt)->getMetric(options->metric));
                else
                {
                    if (*evt == selected_event)
                        color = Qt::yellow;
                    else
                        color = QColor(200, 200, 255);
                }

                bars.append(x);
                bars.append(y);
                bars.append(x);
                bars.append(y + barheight);
                bars.append(x + w);
                bars.append(y + barheight);
                bars.append(x + w);
                bars.append(y);
                for (int j = 0; j < 4; ++j)
                {
                    colors.append(color.red() / 255.0);
                    colors.append(color.green() / 255.0);
                    colors.append(color.blue() / 255.0);
                }

            }
        }
    }

    // Draw
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColorPointer(3,GL_FLOAT,0,colors.constData());
    glVertexPointer(2,GL_FLOAT,0,bars.constData());
    glDrawArrays(GL_QUADS,0,bars.size()/2);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    stepSpan = stopStep - startStep;
}


void TraditionalVis::qtPaint(QPainter *painter)
{
    if(!visProcessed)
        return;

    if ((rect().height() - timescaleHeight) / processSpan >= 3)
        paintEvents(painter);

    drawProcessLabels(painter, rect().height() - timescaleHeight,
                      processheight);
    drawTimescale(painter, startTime, timeSpan);
    drawHover(painter);

    if (!lassoRect.isNull())
    {
        painter->setPen(Qt::yellow);
        painter->drawRect(lassoRect);
        painter->fillRect(lassoRect, QBrush(QColor(255, 255, 144, 150)));
    }
}

void TraditionalVis::paintEvents(QPainter *painter)
{
    int canvasHeight = rect().height() - timescaleHeight;

    int process_spacing = 0;
    if (canvasHeight / processSpan > 12)
        process_spacing = 3;

    float x, y, w, h;
    float blockheight = floor(canvasHeight / processSpan);
    float barheight = blockheight - process_spacing;
    processheight = blockheight;
    int upperStep = startStep + stepSpan + 2;
    startStep = maxStep;
    int stopStep = 0;
    QRect extents = QRect(labelWidth, 0, rect().width(), canvasHeight);

    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = painter->fontMetrics();

    int position, step;
    bool complete;
    QSet<Message *> drawMessages = QSet<Message *>();
    unsigned long long stopTime = startTime + timeSpan;
    painter->setPen(QPen(QColor(0, 0, 0)));
    Partition * part = NULL;

    int start = std::max(int(floor(startProcess)), 0);
    int end = std::min(int(ceil(startProcess + processSpan)),
                       trace->num_processes - 1);
    for (int i = start; i <= end; ++i)
    {
        position = order_to_proc[i];
        QVector<Event *> * roots = trace->roots->at(i);
        for (QVector<Event *>::Iterator root = roots->begin();
             root != roots->end(); ++root)
        {
            paintNotStepEvents(painter, *root, position, process_spacing,
                               barheight, blockheight, &extents);
        }
    }


    for (int i = startPartition; i < trace->partitions->length(); ++i)
    {
        part = trace->partitions->at(i);
        if (part->min_global_step > upperStep)
            break;
        for (QMap<int, QList<Event *> *>::Iterator event_list
             = part->events->begin(); event_list != part->events->end();
             ++event_list)
        {
            bool selected = false;
            if (part->gnome == selected_gnome
                && selected_processes.contains(proc_to_order[event_list.key()]))
            {
                selected = true;
            }

            position = proc_to_order[event_list.key()];
            // Out of process span test
           if (position < floor(startProcess)
                   || position > ceil(startProcess + processSpan))
               continue;

            for (QList<Event *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                 // Out of time span test
                if ((*evt)->exit < startTime || (*evt)->enter > stopTime)
                    continue;


                // save step information for emitting
                step = (*evt)->step;
                if (step >= 0 && step > stopStep)
                    stopStep = step;
                if (step >= 0 && step < startStep) {
                    startStep = step;
                }


                w = ((*evt)->exit - (*evt)->enter) / 1.0
                        / timeSpan * rect().width();
                if (w >= 2)
                {
                    y = floor((position - startProcess) * blockheight) + 1;
                    x = floor(static_cast<long long>((*evt)->enter - startTime)
                              / 1.0 / timeSpan * rect().width()) + 1 + labelWidth;
                    h = barheight;


                    // Corrections for partially drawn
                    complete = true;
                    if (y < 0) {
                        h = barheight - fabs(y);
                        y = 0;
                        complete = false;
                    } else if (y + barheight > canvasHeight) {
                        h = canvasHeight - y;
                        complete = false;
                    }
                    if (x < labelWidth) {
                        w -= (labelWidth - x);
                        x = labelWidth;
                        complete = false;
                    } else if (x + w > rect().width()) {
                        w = rect().width() - x;
                        complete = false;
                    }


                    // Change pen color if selected
                    if (*evt == selected_event && !selected_aggregate)
                        painter->setPen(QPen(Qt::yellow));

                    if (options->colorTraditionalByMetric
                        && (*evt)->hasMetric(options->metric))
                    {
                            painter->fillRect(QRectF(x, y, w, h),
                                              QBrush(options->colormap->color((*evt)->getMetric(options->metric))));
                    }
                    else
                    {
                        if (*evt == selected_event && !selected_aggregate)
                            painter->fillRect(QRectF(x, y, w, h),
                                              QBrush(Qt::yellow));
                        else
                            // Draw event
                            painter->fillRect(QRectF(x, y, w, h),
                                              QBrush(QColor(200, 200, 255)));
                    }

                    // Draw border
                    if (process_spacing > 0)
                    {
                        if (complete)
                            painter->drawRect(QRectF(x,y,w,h));
                        else
                            incompleteBox(painter, x, y, w, h, &extents);
                    }

                    // Revert pen color
                    if (*evt == selected_event && !selected_aggregate)
                        painter->setPen(QPen(QColor(0, 0, 0)));

                    drawnEvents[*evt] = QRect(x, y, w, h);

                    QString fxnName = ((*(trace->functions))[(*evt)->function])->name;
                    QRect fxnRect = font_metrics.boundingRect(fxnName);
                    if (fxnRect.width() < w && fxnRect.height() < h)
                        painter->drawText(x + 2, y + fxnRect.height(), fxnName);

                    // Selected aggregate
                    if (*evt == selected_event && selected_aggregate)
                    {
                        int xa = labelWidth;
                        if ((*evt)->comm_prev)
                            xa = floor(static_cast<long long>((*evt)->comm_prev->exit - startTime)
                                       / 1.0 / timeSpan * rect().width()) + 1 + labelWidth;
                        int wa = x - xa;
                        painter->setPen(QPen(Qt::yellow));
                        painter->drawRect(xa, y, wa, h);
                        painter->setPen(QPen(QColor(0, 0, 0)));
                    }

                }

                for (QVector<Message *>::Iterator msg
                     = (*evt)->messages->begin();
                     msg != (*evt)->messages->end(); ++msg)
                {
                    drawMessages.insert((*msg));
                }
            }
        }
    }

    // Messages
    // We need to do all of the message drawing after the event drawing
    // for overlap purposes
    if (options->showMessages != VisOptions::NONE)
    {
        if (processSpan <= 32)
            painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
        else
            painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
        Event * send_event;
        Event * recv_event;
        QPointF p1, p2;
        for (QSet<Message *>::Iterator msg = drawMessages.begin();
             msg != drawMessages.end(); ++msg)
        {
            send_event = (*msg)->sender;
            recv_event = (*msg)->receiver;
            position = proc_to_order[send_event->process];
            y = floor((position - startProcess + 0.5) * blockheight) + 1;
            x = floor(static_cast<long long>(send_event->enter - startTime)
                      / 1.0 / timeSpan * rect().width()) + 1 + labelWidth;
            p1 = QPointF(x, y);
            position = proc_to_order[recv_event->process];
            y = floor((position - startProcess + 0.5) * blockheight) + 1;
            x = floor(static_cast<long long>(recv_event->exit - startTime)
                      / 1.0 / timeSpan * rect().width()) + 1 + labelWidth;
            p2 = QPointF(x, y);
            painter->drawLine(p1, p2);
        }
    }

    stepSpan = stopStep - startStep;
    painter->fillRect(QRectF(0,canvasHeight,rect().width(),rect().height()),
                      QBrush(QColor(Qt::white)));
}

// To make sure events have correct overlapping, this draws from the root down
// TODO: Have a level cut off so we don't descend all the way down the tree
void TraditionalVis::paintNotStepEvents(QPainter *painter, Event * evt,
                                        float position, int process_spacing,
                                        float barheight, float blockheight,
                                        QRect * extents)
{
    if (evt->enter > startTime + timeSpan || evt->exit < startTime)
        return; // Out of time
    if (evt->step >= 0)
        return; // Drawn later

    int x, y, w, h;
    w = (evt->exit - evt->enter) / 1.0 / timeSpan * rect().width();
    if (w >= 2) // Don't draw tiny events
    {
        y = floor((position - startProcess) * blockheight) + 1;
        x = floor(static_cast<long long>(evt->enter - startTime) / 1.0
                  / timeSpan * rect().width()) + 1 + labelWidth;
        h = barheight;


        // Corrections for partially drawn
        bool complete = true;
        if (y < 0) {
            h = barheight - fabs(y);
            y = 0;
            complete = false;
        } else if (y + barheight > rect().height() - timescaleHeight) {
            h = rect().height() - timescaleHeight - y;
            complete = false;
        }
        if (x < labelWidth) {
            w -= (labelWidth - x);
            x = labelWidth;
            complete = false;
        } else if (x + w > rect().width()) {
            w = rect().width() - x;
            complete = false;
        }


        // Change pen color if selected
        if (evt == selected_event)
            painter->setPen(QPen(Qt::yellow));

        if (options->colorTraditionalByMetric
                && evt->hasMetric(options->metric))
        {
                painter->fillRect(QRectF(x, y, w, h),
                                  QBrush(options->colormap->color(evt->getMetric(options->metric))));
        }
        else
        {
            if (evt == selected_event)
                painter->fillRect(QRectF(x, y, w, h), QBrush(Qt::yellow));
            else
            {
                // Draw event
                int graycolor = std::max(100, 100 + evt->depth * 20);
                painter->fillRect(QRectF(x, y, w, h),
                                  QBrush(QColor(graycolor, graycolor, graycolor)));
            }
        }

        // Draw border
        if (process_spacing > 0)
        {
            if (complete)
                painter->drawRect(QRectF(x,y,w,h));
            else
                incompleteBox(painter, x, y, w, h, extents);
        }

        // Revert pen color
        if (evt == selected_event)
            painter->setPen(QPen(QColor(0, 0, 0)));

        // Replace this with something else that handles tree
        //drawnEvents[*evt] = QRect(x, y, w, h);

        QString fxnName = ((*(trace->functions))[evt->function])->name;
        QRect fxnRect = painter->fontMetrics().boundingRect(fxnName);
        if (fxnRect.width() < w && fxnRect.height() < h)
            painter->drawText(x + 2, y + fxnRect.height(), fxnName);
    }

    for (QVector<Event *>::Iterator child = evt->callees->begin();
         child != evt->callees->end(); ++child)
    {
        paintNotStepEvents(painter, *child, position, process_spacing,
                           barheight, blockheight, extents);
    }


}
