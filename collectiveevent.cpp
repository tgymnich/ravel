#include "collectiveevent.h"

CollectiveEvent::CollectiveEvent(unsigned long long _enter,
                                 unsigned long long _exit,
                                 int _function, int _process, int _phase,
                                 CollectiveRecord *_collective)
    : CommEvent(_enter, _exit, _function, _process, _phase),
      collective(_collective)
{
}

CollectiveEvent::~CollectiveEvent()
{
    delete collective;
}

void CollectiveEvent::set_stride_relationships(CommEvent * base)
{
    // Add to everyone in the collective
    // as a child. This will force the collective to be after
    // anything that happens before any of the collectives.
    for (QList<CollectiveEvent *>::Iterator ev
         = collective->events->begin();
         ev != collective->events->end(); ++ev)
    {
        base->stride_children->insert(*ev);
        (*ev)->stride_parents->insert(base);
    }
}

void CollectiveEvent::mergeForMessagesHelper(QSet<Partition *> * to_merge,
                                             QQueue<Partition *> * to_process)
{
    if (collective->mark)
    {
        for (QList<CollectiveEvent *>::Iterator ev2
             = collective->events->begin();
             ev2 != collective->events->end(); ++ev2)
        {
            to_merge->insert((*ev2)->partition);
            if (!(*ev2)->partition->mark
                    && !to_process->contains((*ev2)->partition))
                to_process->enqueue((*ev2)->partition);
        }

        // Mark so we don't have to do the above again
        (*evt)->collective->mark = true;
    }
}
