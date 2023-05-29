#include <Urho3D/Urho3D.h>

#include "MemoryObjects.h"

void BlitCopy(BlitInfo& i, const IntRect& rect)
{
    i.Clip(rect.left_, rect.top_);

    if (!i.msk)
        CopyFast(i.dst_start, i.src, i.dst_w * i.numbytes, i.src_w * i.numbytes, i.src_skip * i.numbytes, i.src_h);
    else
    {
        if (i.numbytes == sizeof(int))
        {
            Mask((int *)i.dst_start, i.msk, i.dst_w, i.src_w, i.src_skip, i.src_h);
            CopyOR((int *)i.dst_start, (int *)i.src, i.dst_w, i.src_w, i.src_skip, i.src_h);
        }
        else
        if (i.numbytes == sizeof(char))
        {
            Mask(i.dst_start, i.msk, i.dst_w, i.src_w, i.src_skip, i.src_h);
            CopyOR(i.dst_start, i.src, i.dst_w, i.src_w, i.src_skip, i.src_h);
        }
    }
}

template class Stack<unsigned int>;
