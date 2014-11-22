package org.bitman.CamToy;

import java.util.Queue;

/**
 * Created by Spartan on 2014/11/21.
 */
public interface ExceptionCallback {
    public void onExceptionHappen(Queue<byte[]> listException);
}
