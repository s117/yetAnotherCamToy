package org.bitman.CamToy;

import java.util.concurrent.Semaphore;

/**
 * Created by Spartan on 2014/11/21.
 */
public class TcpAsyncSession {
    public byte[] request;
    public byte[] reply;
    public volatile Semaphore semProcess;
    public volatile int status;

    public TcpAsyncSession(int lenTx) {
        semProcess = new Semaphore(0);
        status = TcpHandler.TCP_LINK_RETURN_OK;
        request = new byte[lenTx];
        reply = new byte[TcpHandler.PACKET_LENGTH];
    }
}
