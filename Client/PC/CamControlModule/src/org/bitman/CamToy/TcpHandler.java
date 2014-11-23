/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package org.bitman.CamToy;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.NotYetConnectedException;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.util.Iterator;
import java.util.Queue;
import java.util.Set;
import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.concurrent.Semaphore;
//import java.util.concurrent.locks.Condition;

/**
 * @author 骜
 */
public class TcpHandler {
    public static final int TCP_LINK_RETURN_OK = 0;
    public static final int TCP_LINK_RETURN_ERROR_SEND = (1 << 31) | (1 << 0);
    public static final int TCP_LINK_RETURN_ERROR_RECV = (1 << 31) | (1 << 1);
    public static final int TCP_LINK_RETURN_ERROR_DISCONNECTION = (1 << 31) | (1 << 2);
    public static final int TCP_LINK_RETURN_ERROR_RECV_TIMEOUT = (1 << 31) | (1 << 3);
    public static final int TCP_LINK_RETURN_ERROR_SERVE_THREAD_NOT_RUNNING = (1 << 31) | (1 << 4);
    protected static final int PACKET_LENGTH = 5;
    private static final int RECV_TIMEOUT = 3000;
    private ExceptionCallback m_exceptionCallback;
    private SocketChannel m_channelTCP;
    private Selector m_selector;
    private SelectionKey m_selKeyThisChannel;
    private ByteBuffer m_RxBuffer;
    private ByteBuffer m_TxBuffer;
    private String m_ipDst;
    private int m_portDst;
    private InetSocketAddress m_addressDst;
    private volatile boolean m_isConnected;
    private volatile boolean m_isOnService;
    private Queue<TcpAsyncSession> m_TxQueue;
    private Semaphore m_TxSemaphore;
    private Queue<TcpAsyncSession> m_RxQueue;
    private Queue<byte[]> m_exceptionQueue;
    private Thread m_TxThread;
    private Thread m_RxThread;


    public TcpHandler(ExceptionCallback exceptionCallback, String TargetAddress, int TargetPort) throws IOException, IllegalStateException {
        m_exceptionCallback = exceptionCallback;
        m_ipDst = TargetAddress;
        m_portDst = TargetPort;
        m_isConnected = false;
        m_addressDst = new InetSocketAddress(m_ipDst, m_portDst);

        m_RxBuffer = ByteBuffer.allocate(2000); // large than normal network's MTU
        m_RxBuffer.clear();
        m_RxBuffer.flip();

        m_TxBuffer = ByteBuffer.allocate(50); // 10 Packet
        m_TxBuffer.clear();
        m_TxBuffer.flip();

        m_isOnService = false;
        m_TxQueue = new ConcurrentLinkedDeque<TcpAsyncSession>();
        m_RxQueue = new ConcurrentLinkedDeque<TcpAsyncSession>();
        m_TxSemaphore = new Semaphore(0);
        m_exceptionQueue = new ConcurrentLinkedDeque<byte[]>();
        m_TxThread = new Thread(new TxWorker());
        m_TxThread.setDaemon(true);
        m_RxThread = new Thread(new RxWorker());
        m_RxThread.setDaemon(true);
    }

    @Override
    protected void finalize() {
        DropConnection();
        try {
            stopAsync();
        } catch (Exception ex) {
            ex.printStackTrace();
        }
        try {
            super.finalize();
        } catch (Throwable throwable) {
        }
    }

    public void startAsync() {
        if (m_isOnService)
            return;
        m_isOnService = true;
        m_TxSemaphore.drainPermits();
        m_TxThread.start();
        m_RxThread.start();
    }

    public void stopAsync() throws InterruptedException {
        if (!m_isOnService)
            return;
        m_isOnService = false;
        m_TxSemaphore.release();
        if (!Thread.currentThread().equals(m_TxThread))
            m_TxThread.join();
        if (!Thread.currentThread().equals(m_RxThread))
            m_RxThread.join();
    }

    public int Link(TcpAsyncSession session) {
        if (!getIsConnected()) {
            return TCP_LINK_RETURN_ERROR_DISCONNECTION;
        }
        if (!m_isOnService) {
            return TCP_LINK_RETURN_ERROR_SERVE_THREAD_NOT_RUNNING;
        }
        m_TxQueue.offer(session);
        m_TxSemaphore.release();
        try {
            session.semProcess.acquire();
        } catch (InterruptedException e) {
            e.printStackTrace();
            return TCP_LINK_RETURN_ERROR_RECV;
        }
        return session.status;
    }

    public boolean EstablishConnection() {
        if (m_isConnected) {
            return true;
        }
        try {
            m_channelTCP = SocketChannel.open();
            m_channelTCP.socket().bind(new InetSocketAddress(0));
            m_channelTCP.configureBlocking(false);
            m_selector = Selector.open();
            m_channelTCP.connect(m_addressDst);
            while (!m_channelTCP.finishConnect()) ;
            m_selKeyThisChannel = m_channelTCP.register(m_selector, SelectionKey.OP_READ, m_RxBuffer);
            m_isConnected = true;
            startAsync();
            return true;
        } catch (IOException ex) {
            return false;
        }
    }

    public boolean DropConnection() {
        if (!m_isConnected) {
            return true;
        }
        try {
            stopAsync();
            m_isConnected = false;
            m_selector.close();
            m_selector = null;
            m_channelTCP.close();
            m_channelTCP = null;
            m_selKeyThisChannel = null;
            return true;
        } catch (Exception ex) {
            //ex.printStackTrace(); //exception_deal  this position will hold the exception
            //because this function may be called when exception has happened
            return false;
        }
    }

    public boolean getIsConnected() {
        return (m_channelTCP != null) && m_channelTCP.isConnected();
        //return m_isConnected;
    }

    public boolean getIsOnService() {
        return m_isOnService;
    }

    public String getIpDst() {
        return m_ipDst;
    }

    public int getPortDst() {
        return m_portDst;
    }

    protected int Recieve() {
        if (!getIsConnected()) {
            return TCP_LINK_RETURN_ERROR_DISCONNECTION;
        }
        Set<SelectionKey> selectionKeysSet;
        Iterator<SelectionKey> keyIterator;
        m_RxBuffer.compact(); // clear for write and count how many date received
        try {
            if (0 == m_selector.select(RECV_TIMEOUT)) {
                m_RxBuffer.flip();// prepare for read
                return TCP_LINK_RETURN_ERROR_RECV_TIMEOUT;
            }
            selectionKeysSet = m_selector.selectedKeys();
            keyIterator = selectionKeysSet.iterator();
            while (keyIterator.hasNext()) {
                SelectionKey key = keyIterator.next();
                if (key.isReadable()) {
                    int rtnVal;
                    rtnVal = ((SocketChannel) key.channel()).read(m_RxBuffer);
                    if (rtnVal < 0) {// reached end-of-stream
                        DropConnection();
                        m_RxBuffer.flip();// prepare for read
                        return TCP_LINK_RETURN_ERROR_DISCONNECTION;
                    }
                    keyIterator.remove();
                }
            }
        } catch (NotYetConnectedException ex) {
            ex.printStackTrace();
            DropConnection();
            m_RxBuffer.flip();
            return TCP_LINK_RETURN_ERROR_DISCONNECTION;
        } catch (IOException ex) {
            ex.printStackTrace();
            m_RxBuffer.flip();
            return TCP_LINK_RETURN_ERROR_RECV;
        }
        m_RxBuffer.flip();// prepare for read
        return TCP_LINK_RETURN_OK;
    }

    protected synchronized int Send(byte[] dat) {
        m_TxBuffer.clear();
        m_TxBuffer.put(dat);
        m_TxBuffer.flip();
        if (!getIsConnected())
            return TCP_LINK_RETURN_ERROR_DISCONNECTION;
        try {
            while (m_TxBuffer.hasRemaining()) {
                m_channelTCP.write(m_TxBuffer);
            }
        } catch (IOException e) {
            return TCP_LINK_RETURN_ERROR_SEND;
        }
        return TCP_LINK_RETURN_OK;
    }

    class RxWorker implements Runnable { // receive routine

        @Override
        public void run() {
            try {
                while (m_isOnService) {
                    int rtnVal;
                    rtnVal = Recieve();
                    if (((rtnVal == TCP_LINK_RETURN_ERROR_DISCONNECTION)||((rtnVal == TCP_LINK_RETURN_ERROR_RECV))) && (m_RxBuffer.limit() - m_RxBuffer.position() < PACKET_LENGTH)) {
                        TcpAsyncSession failSession;
                        while (!m_RxQueue.isEmpty()) {
                            failSession = m_RxQueue.poll();
                            failSession.status = TCP_LINK_RETURN_ERROR_DISCONNECTION;
                            failSession.semProcess.release();
                        }
                        continue;
                    }
                    while (m_RxBuffer.limit() - m_RxBuffer.position() >= PACKET_LENGTH) {
                        if (m_RxQueue.isEmpty()) { // no request is corresponding, so process it as a exception info
                            byte[] packet = new byte[PACKET_LENGTH];
                            m_RxBuffer.get(packet, 0, PACKET_LENGTH);
                            m_exceptionQueue.offer(packet);
                            m_exceptionCallback.onExceptionHappen(m_exceptionQueue);
                        } else {
                            TcpAsyncSession curProceeding = m_RxQueue.poll();
                            curProceeding.status |= rtnVal;
                            m_RxBuffer.get(curProceeding.reply, 0, PACKET_LENGTH);
                            curProceeding.semProcess.release();
                        }
                    }
                }
            } catch (Exception ex) {
                ex.printStackTrace();
                DropConnection();
            }
        }
    }

    class TxWorker implements Runnable { // sending routine

        @Override
        public void run() {
            try {
                while (m_isOnService) {
                    try {
                        m_TxSemaphore.acquire();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    if (!getIsConnected()) {
                        TcpAsyncSession failSession;
                        while (!m_TxQueue.isEmpty()) {
                            failSession = m_TxQueue.poll();
                            failSession.status = TCP_LINK_RETURN_ERROR_DISCONNECTION;
                            failSession.semProcess.release();
                        }
                        continue;
                    }
                    if (m_TxQueue.isEmpty())
                        continue;
                    TcpAsyncSession curProceeding = m_TxQueue.poll();
                    m_RxQueue.offer(curProceeding);
                    curProceeding.status |= Send(curProceeding.request);
                }
            } catch (Exception ex) {
                ex.printStackTrace();
                DropConnection();
            }
        }
    }

}
