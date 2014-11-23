package org.bitman.CamToy;

import java.security.MessageDigest;
import java.text.SimpleDateFormat;
import java.util.*;

/**
 * Created by Spartan on 2014/11/10.
 */


public class CommModule implements ExceptionCallback {
    public static final int COMM_UNINITIALIZED = 0;
    public static final int COMM_OK = (1 << 0);
    public static final int COMM_NEED_AUTH = (1 << 1);
    public static final int COMM_NEED_NOT_AUTH = (1 << 2);
    public static final int COMM_ERROR_CONNECTION_FAIL = (1 << 31) | (1 << 3);
    public static final int COMM_ERROR_LOGIN_FAIL = (1 << 31) | (1 << 4);
    public static final int COMM_ERROR_SERIAL_OCCUPY = (1 << 31) | (1 << 5);
    public static final int COMM_ERROR_SERIAL_OPEN_FAIL = (1 << 31) | (1 << 6);
    public static final int COMM_ERROR_TIMEOUT = (1 << 31) | (1 << 7);
    public static final int COMM_ERROR_ILLEGAL_INST = (1 << 31) | (1 << 8);

    public static final int SERVO_OK = (0);
    public static final int SERVO_FAIL = (1 << 1);           // 1 Servo board report error
    public static final int SERVO_OFFLINE = (1 << 2);        // 2 Servo offline
    public static final int SERVO_BADLINK = (1 << 3);        // 4 Serial transmission unstable
    public static final int SERVO_LINKDOWN = (1 << 4);       // 8 Serial port operate fail

    public static final int COMM_WARN_SERVO_UNSTABLE = (1 << 30) | (1 << 9);

    public static final int SERVO_MOVE_DIRECTION_POSE = 1;
    public static final int SERVO_MOVE_DIRECTION_STOP = 0;
    public static final int SERVO_MOVE_DIRECTION_NEGA = 2;
    public static final byte[] COMM_INST_DROP = {'C', 'D', '\0', '\0', '\0'};
    public static final byte[] COMM_INST_KEEP = {'C', 'K', '\0', '\0', '\0'};
    public TcpHandler m_LinkHanlder;
    private volatile boolean m_isAuth;
    private volatile boolean m_isConnected;
    private volatile int m_ServerNotify;
    private volatile byte m_ServoDetail;
    private int m_ServerKeepAliveMin;
    private int m_ServerKeepAliveSec;
    private Timer m_ServerKeepAliveTimer;

    /**
     * Create a new Communicate module
     */
    public CommModule() {
        m_LinkHanlder = null;
        m_isAuth = false;
        m_isConnected = false;
        m_ServerNotify = COMM_UNINITIALIZED;
    }

    protected static byte[] MD5(String s) {
        try {
            byte[] btInput = s.getBytes();
            MessageDigest mdInst = MessageDigest.getInstance("MD5");
            mdInst.update(btInput);
            return mdInst.digest();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    protected static String FormatAuthMidder() {
        Calendar now = Calendar.getInstance();
        now.setTimeZone(TimeZone.getTimeZone("GMT"));
        SimpleDateFormat dataFormat = new SimpleDateFormat(":MM:dd:");
        dataFormat.setTimeZone(TimeZone.getTimeZone("GMT"));
        int hour = now.get(Calendar.HOUR_OF_DAY);
        return ":0" + (now.get(Calendar.YEAR) - 1900) + dataFormat.format(now.getTime()) + (now.get(Calendar.DAY_OF_WEEK)) + ":" + ((hour >= 10) ? "" : "0") + hour + ":";
    }

    /**
     * Internal method, use for handle server push notify, you can override this method to change the reaction
     *
     * @param listException a thread safe queue which contain message pushed by server
     */
    @Override
    public void onExceptionHappen(Queue<byte[]> listException) {
        while (!listException.isEmpty()) {
            byte[] curMsg = listException.poll();
            switch (AnalyseServerMsg(curMsg)) {
                case MSG_HELLO_INFO:
                    m_ServerKeepAliveMin = curMsg[2];
                    m_ServerKeepAliveSec = curMsg[3];
                    m_isConnected = true;
                    if (curMsg[4] == 0) {
                        m_isAuth = true;
                        m_ServerNotify |= COMM_NEED_NOT_AUTH;
                    } else {
                        m_isAuth = false;
                        m_ServerNotify |= COMM_NEED_AUTH;
                    }
                    break;
                case MSG_ERROR_LOGIN:
                    break;
                case MSG_ERROR_SERIAL_OPEN_FAIL:
                    m_isConnected = false;
                    m_ServerNotify |= COMM_ERROR_SERIAL_OPEN_FAIL;
                    m_LinkHanlder.DropConnection();
                    break;
                case MSG_ERROR_SERIAL_OCCUPY:
                    m_isConnected = false;
                    m_ServerNotify |= COMM_ERROR_SERIAL_OCCUPY;
                    m_LinkHanlder.DropConnection();
                    break;
                case MSG_ERROR_TIMEOUT:
                    m_isConnected = false;
                    m_ServerNotify |= COMM_ERROR_TIMEOUT;
                    m_LinkHanlder.DropConnection();
                    break;
                case MSG_ERROR_ILLEGAL_INST:
                    break;
                case MSG_ERROR_SERVO_UNSTABLE:
                    m_ServoDetail = curMsg[4];
                    break;
                case MSG_OK:
                    break;
                case MSG_INVALID:
                    m_ServerNotify |= COMM_ERROR_CONNECTION_FAIL;
                    m_LinkHanlder.DropConnection();
                    break;
            }

        }
    }

    /**
     * @param targetAddress Server's IPv4 address
     * @param targetPort    Server's listening port
     * @return When success, return:<br/>
     * COMM_NEED_AUTH for server need authorization<br/>
     * COMM_NEED_NOT_AUTH for needn't.<br/>
     * <br/>
     * When fail, return COMM_ERROR_CONNECTION_FAIL.
     */
    public int Connect(String targetAddress, int targetPort) {
        try {
            m_LinkHanlder = new TcpHandler(this, targetAddress, targetPort);
            if (!m_LinkHanlder.EstablishConnection()) {
                m_LinkHanlder.DropConnection();
                m_LinkHanlder.stopAsync();
                return COMM_ERROR_CONNECTION_FAIL;
            } else {
                m_LinkHanlder.startAsync();
            }

            while (m_ServerNotify == COMM_UNINITIALIZED) ;

            if (m_ServerNotify < 0) {
                m_LinkHanlder.DropConnection();
                m_LinkHanlder.stopAsync();
                m_ServerNotify = COMM_UNINITIALIZED;
                return COMM_ERROR_CONNECTION_FAIL;
            } else if (((m_ServerNotify & COMM_NEED_NOT_AUTH) != 0)) {
                m_ServerNotify = COMM_UNINITIALIZED;
                return COMM_NEED_NOT_AUTH;
            } else {
                m_ServerNotify = COMM_UNINITIALIZED;
                return COMM_NEED_AUTH;
            }
        } catch (Exception ex) {// fail to create a TCP_Link instance
            ex.printStackTrace();
            return COMM_ERROR_CONNECTION_FAIL;
        }
    }

    /**
     * @param user Username
     * @param pswd Password
     * @return When success, return COMM_OK. <br/>
     * When fail return a negative number:<br/>
     * 1、if have chance return (0 - last_chance).<br/>
     * 2、if have no chance return COMM_ERROR_LOGIN_FAIL.<br/>
     * 3、if because network reason return COMM_ERROR_CONNECTION_FAIL.<br/>
     * 4、if authorize success, but other user has blocked server's serial port, return COMM_ERROR_SERIAL_OCCUPY<br/>
     * ATTENTION: Return COMM_ERROR_CONNECTION_FAIL or COMM_ERROR_SERIAL_OCCUPY means you need perform a reconnection!
     */
    public int Authorize(String user, String pswd) {
        if (m_isAuth)
            return COMM_OK;
        if (!m_LinkHanlder.getIsConnected()) {
            m_LinkHanlder.DropConnection();
            return COMM_ERROR_CONNECTION_FAIL;// link lost
        }
        TcpAsyncSession authSession = new TcpAsyncSession(16);
        System.arraycopy(MD5(user + FormatAuthMidder() + pswd), 0, authSession.request, 0, 16);

        int rtnVal = m_LinkHanlder.Link(authSession);
        if (rtnVal >= 0) {
            switch (AnalyseServerMsg(authSession.reply)) {
                case MSG_ERROR_LOGIN:// wrong proof
                    if (authSession.reply[4] == 0) {
                        m_LinkHanlder.DropConnection();
                        return COMM_ERROR_LOGIN_FAIL;// has no chance
                    } else
                        return -authSession.reply[4];// has chance
                case MSG_ERROR_SERIAL_OCCUPY:
                    m_LinkHanlder.DropConnection();
                    return COMM_ERROR_SERIAL_OCCUPY;
                case MSG_OK:
                    m_isAuth = true;
                    return COMM_OK;
                default:
                    m_LinkHanlder.DropConnection();  // get other reply server are abnormal
                    return COMM_ERROR_CONNECTION_FAIL;
            }
        } else {
            m_LinkHanlder.DropConnection();
            return COMM_ERROR_CONNECTION_FAIL;
        }
    }

    /**
     * Use this method to query servo server's state
     *
     * @param type what type of data you wanna to get
     * @param axis which axis
     * @return When success, return value replied by server, this value is a 8-bit unsigned number from 0x00~0xff. <br/>
     * When fail return a negative number:<br/>
     * 1、if you give a invalid parameter, will return COMM_ILLEGAL_INST.<br/>
     * 2、if network counter a failure, return COMM_ERROR_CONNECTION_FAIL.<br/>
     * <br/>
     * ATTENTION: Return COMM_ERROR_CONNECTION_FAIL means you need perform a reconnection!
     */
    public int Query(ClientReqType type, byte axis) {
        if ((axis != 'X') && (axis != 'Y'))
            return COMM_ERROR_ILLEGAL_INST;
        TcpAsyncSession session = new TcpAsyncSession(TcpHandler.PACKET_LENGTH);

        session.request[0] = 'C';
        session.request[1] = 'Q';
        session.request[3] = axis;
        session.request[4] = '\0';
        switch (type) {
            case BALANCE:
                session.request[2] = 'B';
                break;
            case ABS_POS:
                session.request[2] = 'A';
                break;
            case MOV_SPEED:
                session.request[2] = 'S';
                break;
            case MOV_STATE:
                session.request[2] = 'M';
                break;
        }
        if (m_LinkHanlder.Link(session) < 0) {
            m_LinkHanlder.DropConnection();
            return COMM_ERROR_CONNECTION_FAIL;
        }
        switch (AnalyseServerMsg(session.reply)) {
            case MSG_ERROR_ILLEGAL_INST:
                return COMM_ERROR_ILLEGAL_INST;
            case MSG_OK:
                if (session.reply[3] == axis) {
                    return session.reply[4];
                } else {
                    m_LinkHanlder.DropConnection();
                    return COMM_ERROR_CONNECTION_FAIL;
                }
            default:// Invalid reply
                m_LinkHanlder.DropConnection();
                return COMM_ERROR_CONNECTION_FAIL;
        }
    }

    /**
     * Use this method to set servo server's state
     *
     * @param type what type of data you wanna to set
     * @param axis which axis
     * @param val  a 8-bit unsigned number from 0x00~0xff
     * @return When success, return value replied by server, this value is a 8-bit unsigned number from 0x00~0xff. <br/>
     * When fail return a negative number:<br/>
     * 1、if you give a invalid parameter, will return COMM_ILLEGAL_INST.<br/>
     * 2、if network counter a failure, return COMM_ERROR_CONNECTION_FAIL.<br/>
     * When server gives a servo status warning, return COMM_OK | COMM_WARN_SERVO_UNSTABLE,it's a large positive number(bit 30 is set)<br/>
     * <br/>
     * ATTENTION: Return COMM_ERROR_CONNECTION_FAIL means you need perform a reconnection!
     */
    public int Set(ClientReqType type, byte axis, byte val) {
        if (((axis != 'X') && (axis != 'Y')) || ((val < 0) || (val > 0xff)))
            return COMM_ERROR_ILLEGAL_INST;
        TcpAsyncSession session = new TcpAsyncSession(TcpHandler.PACKET_LENGTH);

        session.request[0] = 'C';
        session.request[1] = 'S';
        session.request[3] = axis;
        session.request[4] = val;
        switch (type) {
            case BALANCE:
                session.request[2] = 'B';
                break;
            case ABS_POS:
                session.request[2] = 'A';
                break;
            case MOV_SPEED:
                session.request[2] = 'S';
                break;
            case MOV_STATE:
                session.request[2] = 'M';
                break;
        }
        if (m_LinkHanlder.Link(session) < 0) {
            m_LinkHanlder.DropConnection();
            return COMM_ERROR_CONNECTION_FAIL;
        }
        switch (AnalyseServerMsg(session.reply)) {
            case MSG_ERROR_ILLEGAL_INST:
                return COMM_ERROR_ILLEGAL_INST;
            case MSG_ERROR_SERVO_UNSTABLE:
                m_ServoDetail = session.reply[4];
                return (COMM_OK | COMM_WARN_SERVO_UNSTABLE);
            case MSG_OK:
                if ((session.reply[3] == axis) && (session.reply[4] == val)) {
                    m_ServoDetail = SERVO_OK;
                    return COMM_OK;
                } else {
                    m_LinkHanlder.DropConnection();
                    return COMM_ERROR_CONNECTION_FAIL;
                }
            default: //Invalid reply
                m_LinkHanlder.DropConnection();
                return COMM_ERROR_CONNECTION_FAIL;
        }
    }

    /**
     * Disconnect normally
     */
    public void Disconnect() {
        if (m_LinkHanlder != null) {
            if (m_LinkHanlder.getIsConnected())
                m_LinkHanlder.Send(COMM_INST_DROP);
            m_LinkHanlder.DropConnection();
            m_LinkHanlder = null;
        }
    }

    /**
     * Send a NOP instruction
     */
    public void SendNOP() {
        if (m_LinkHanlder.getIsConnected()) {
            TcpAsyncSession session = new TcpAsyncSession(TcpHandler.PACKET_LENGTH);
            System.arraycopy(COMM_INST_KEEP, 0, session.request, 0, 5);
            m_LinkHanlder.Link(session);
            switch (AnalyseServerMsg(session.reply)) {
                case MSG_ERROR_SERVO_UNSTABLE:
                    m_ServoDetail = session.reply[4];
                    break;
                case MSG_OK:
                    m_ServoDetail = SERVO_OK;
                    break;
                default: //Invalid reply
                    //TODO: This place can cause a connection lost
                    m_LinkHanlder.DropConnection();
                    break;
            }
        }
    }

    /**
     * Enable keep alive timer, you need enable it manually, but it will stop automatic when no more need.
     */
    public void EnableKeepAlive() {
        long interval = ((m_ServerKeepAliveSec + m_ServerKeepAliveMin * 60) * 1000);
        if (interval <= 0)
            return;// server return a fucking value!
        if (m_ServerKeepAliveTimer == null) {
            m_ServerKeepAliveTimer = new Timer("Connection keep alive task", true);
        }
        int decFactor;
        for (decFactor = 10000; interval - decFactor <= 0; decFactor = decFactor / 2) ;
        interval = interval - decFactor;
        m_ServerKeepAliveTimer.scheduleAtFixedRate(new KeepAliveRoutine(), 0, interval);

    }

    /**
     * Disable keep alive timer manually.
     */
    public void DisableKeepAlive() {
        if (m_ServerKeepAliveTimer != null) {
            m_ServerKeepAliveTimer.cancel();
        }
    }

    /**
     * Get detail about what exception servo reported.
     *
     * @param errno error code
     * @return Detail string
     */
    public String GetServoExceptionByErrno(int errno) {
        String detail;
        if (errno == SERVO_OK) {
            detail = "Servo system working finely.";
        } else {
            detail = "Servo system meets following problem:\n";
            if ((errno & SERVO_FAIL) != 0) {
                detail += "\tServo drive system report an error.\n";
            }
            if ((errno & SERVO_OFFLINE) != 0) {
                detail += "\tServo offline.\n";
            }
            if ((errno & SERVO_BADLINK) != 0) {
                detail += "\tSerial transmission unstable.\n";
            }
            if ((errno & SERVO_LINKDOWN) != 0) {
                detail += "\tSerial port operate fail.\n";
            }
        }
        return detail;
    }

    /**
     * Get error code about servo.
     *
     * @return
     * server's status
     */
    public int GetServoStatus() {
        return m_ServoDetail;
    }

    protected ServerMsg AnalyseServerMsg(byte[] srvReply) {
        if (srvReply[0] != 'S') {
            return ServerMsg.MSG_INVALID;
        }
        if (srvReply[1] == 'O') {// Error type
            if (srvReply[2] == 'K')
                return ServerMsg.MSG_OK;
            else
                return ServerMsg.MSG_INVALID;
        } else if (srvReply[1] == 'E') {// Error type
            switch (srvReply[2]) {
                case 'L':// Login type
                    if (srvReply[3] == 'F')
                        return ServerMsg.MSG_ERROR_LOGIN;
                    else
                        return ServerMsg.MSG_INVALID;
                    //break;
                case 'C'://Serial type
                    if (srvReply[3] == 'F')
                        return ServerMsg.MSG_ERROR_SERIAL_OPEN_FAIL;
                    else if (srvReply[3] == 'O')
                        return ServerMsg.MSG_ERROR_SERIAL_OCCUPY;
                    else
                        return ServerMsg.MSG_INVALID;
                    //break;
                case 'T'://Time type
                    if (srvReply[3] == 'O')
                        return ServerMsg.MSG_ERROR_TIMEOUT;
                    else
                        return ServerMsg.MSG_INVALID;
                    //break;
                case 'I':// Illegal type
                    if (srvReply[3] == 'I')
                        return ServerMsg.MSG_ERROR_ILLEGAL_INST;
                    else
                        return ServerMsg.MSG_INVALID;
                    //break;
                case 'S':// Servo type
                    if (srvReply[3] == 'E')
                        return ServerMsg.MSG_ERROR_SERVO_UNSTABLE;
                    else
                        return ServerMsg.MSG_INVALID;
                    //break;
                default:
                    return ServerMsg.MSG_INVALID;
            }
        } else if (srvReply[1] == 'H') {
            return ServerMsg.MSG_HELLO_INFO;
        } else {
            return ServerMsg.MSG_INVALID;
        }
    }

    public enum ServerMsg {
        MSG_HELLO_INFO,
        MSG_ERROR_LOGIN,
        MSG_ERROR_SERIAL_OPEN_FAIL,
        MSG_ERROR_SERIAL_OCCUPY,
        MSG_ERROR_TIMEOUT,
        MSG_ERROR_ILLEGAL_INST,
        MSG_ERROR_SERVO_UNSTABLE,
        MSG_OK,
        MSG_INVALID
    }

    public enum ClientReqType {
        BALANCE,
        ABS_POS,
        MOV_SPEED,
        MOV_STATE
    }

    class KeepAliveRoutine extends TimerTask {

        @Override
        public void run() {
            if (m_LinkHanlder.getIsConnected()) {
                if (m_isAuth) {
                    SendNOP();
                }
            } else {
                m_ServerKeepAliveTimer.cancel();
            }
        }
    }
}
