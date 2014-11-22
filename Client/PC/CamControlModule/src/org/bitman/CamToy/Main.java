package org.bitman.CamToy;

import java.util.Scanner;

public class Main {

    static Scanner sc;
    static CommModule module;

    public static void main(String[] args) {
        // write your code here
//        for(int i= 0;i < CommModule.COMM_INST_DROP.length;++i){
//            System.out.println("KEEP["+i+"]="+CommModule.COMM_INST_DROP[i]);
//        }
//
//        for(int i=0;i <CommModule.COMM_INST_KEEP.length;++i){
//            System.out.println("KEEP["+i+"]="+CommModule.COMM_INST_KEEP[i]);
//        }

        sc = (new Scanner(System.in));

        module = new CommModule();

        Connection();
        StartRootMenu();

    }

    public static void Exit() {
        System.out.println("Exiting...");
        module.Disconnect();
        System.exit(0);
    }


    public static void Connection() {
        int rtnVal;
        rtnVal = module.Connect("10.50.1.2", 1736);
        switch (rtnVal) {
            case CommModule.COMM_NEED_AUTH:
                rtnVal = CommModule.COMM_UNINITIALIZED;
                while (rtnVal != CommModule.COMM_OK) {
                    System.out.println("Connection success, need authenticate");
                    System.out.print("User:");
                    String user = sc.nextLine();
                    System.out.print("Pass:");
                    String pass = sc.nextLine();
                    rtnVal = (module.Authorize(user, pass));
                    switch (rtnVal) {
                        case CommModule.COMM_OK:
                            System.out.println("Authenticate success.");
                            module.EnableKeepAlive();
                            break;
                        case CommModule.COMM_ERROR_LOGIN_FAIL:
                            System.out.println("Authenticate fail, you have no chance, connection lost.");
                            Exit();
                            break;
                        case CommModule.COMM_ERROR_CONNECTION_FAIL:
                            System.out.println("Connection error.");
                            Exit();
                            break;
                        case CommModule.COMM_ERROR_SERIAL_OCCUPY:
                            System.out.println("Authenticate success, but server report there is another user using, connection lost.");
                            Exit();
                            break;
                        default:
                            System.out.println("Authenticate fail, last chance: " + (-rtnVal));
                            break;
                    }
                }
                break;
            case CommModule.COMM_NEED_NOT_AUTH:
                System.out.println("Server needn't authenticate.");
                break;
            case CommModule.COMM_ERROR_CONNECTION_FAIL:
                System.out.println("Connection error.");
                Exit();
                break;
        }
    }

    public static void StartRootMenu() {
        while (true) {
            System.out.println("\nSelect operation:");
            System.out.println("\t'S' -- (S)etting mode");
            System.out.println("\t'Q' -- (Q)uery mode");
            System.out.println("\t'T' -- show servo s(T)ate");
            System.out.println("\t'X' -- disconnect and e(X)it");
            String op = sc.nextLine();
            if (op.equalsIgnoreCase("S")) {
                StartSettingMenu();
            } else if (op.equalsIgnoreCase("Q")) {
                StartQueryMenu();
            } else if (op.equalsIgnoreCase("T")) {
                System.out.println(module.GetServoExceptionByErrno(module.GetServoStatus()));
            } else if (op.equalsIgnoreCase("X")) {
                System.out.println("Exiting...");
                module.Disconnect();
                return;
            } else {
                System.out.println("Invalid input.");
            }
        }
    }

    public static void StartSettingMenu() {
        byte op;
        while (true) {
            System.out.println("\nSetting mode menu:");
            System.out.println("\t'B' -- set (B)alance position");
            System.out.println("\t'A' -- set (A)bsolute position");
            System.out.println("\t'S' -- set moving (S)peed");
            System.out.println("\t'M' -- set (M)oving state");
            System.out.println("\t'X' -- e(X)it setting mode");
            op = Getchar();
            switch (op) {
                case 'B':
                case 'b':
                    SetAndPrint(CommModule.ClientReqType.BALANCE, GetAxis(), GetVal());
                    break;
                case 'A':
                case 'a':
                    SetAndPrint(CommModule.ClientReqType.ABS_POS, GetAxis(), GetVal());
                    break;
                case 'S':
                case 's':
                    SetAndPrint(CommModule.ClientReqType.MOV_SPEED, GetAxis(), GetVal());
                    break;
                case 'M':
                case 'm':
                    SetAndPrint(CommModule.ClientReqType.MOV_STATE, GetAxis(), GetStat());
                    break;
                case 'X':
                case 'x':
                    return;
                default:
                    System.out.println("Invalid input.");
            }
        }
    }

    public static void StartQueryMenu() {
        byte op;
        while (true) {
            System.out.println("\nQuery mode menu:");
            System.out.println("\t'B' -- query (B)alance position");
            System.out.println("\t'A' -- query (A)bsolute position");
            System.out.println("\t'S' -- query moving (S)peed");
            System.out.println("\t'M' -- query (M)oving state");
            System.out.println("\t'X' -- e(X)it query mode");
            op = Getchar();
            switch (op) {
                case 'B':
                case 'b':
                    QueryAndPrint(CommModule.ClientReqType.BALANCE, GetAxis());
                    break;
                case 'A':
                case 'a':
                    QueryAndPrint(CommModule.ClientReqType.ABS_POS, GetAxis());
                    break;
                case 'S':
                case 's':
                    QueryAndPrint(CommModule.ClientReqType.MOV_SPEED, GetAxis());
                    break;
                case 'M':
                case 'm':
                    QueryAndPrint(CommModule.ClientReqType.MOV_STATE, GetAxis());
                    break;
                case 'X':
                case 'x':
                    return;
                default:
                    System.out.println("Invalid input.");
            }
        }
    }

    public static void QueryAndPrint(CommModule.ClientReqType type, byte axis) {
        int rtnVal = module.Query(type, axis);
        if (rtnVal == CommModule.COMM_ERROR_CONNECTION_FAIL) {
            System.out.println("Connection lost.");
            System.out.println("would you want to reconnect? y/N");
            byte input = Getchar();
            if ((input == 'Y') || (input == 'y')) {
                Connection();
            } else {
                Exit();
            }
        } else if (rtnVal == CommModule.COMM_ERROR_ILLEGAL_INST) {
            System.out.println("Server report that it was received a illegal instruction, query fail.");
        } else {
            System.out.println("Server Value: " + rtnVal);
        }
    }

    public static void SetAndPrint(CommModule.ClientReqType type, byte axis, byte val) {
        int rtnVal = module.Set(type, axis, val);
        if (rtnVal == CommModule.COMM_ERROR_CONNECTION_FAIL) {
            System.out.println("Connection lost.");
            System.out.println("would you want to reconnect? y/N");
            byte input = Getchar();
            if ((input == 'Y') || (input == 'y')) {
                Connection();
            } else {
                Exit();
            }
        } else if (rtnVal == CommModule.COMM_ERROR_ILLEGAL_INST) {
            System.out.println("Server report that it was received a illegal instruction, query fail.");
        } else if (rtnVal == CommModule.COMM_WARN_SERVO_UNSTABLE) {
            System.out.println("Server has accepted the operation, but report servo system unstable");
            System.out.println(module.GetServoExceptionByErrno(module.GetServoStatus()));
        } else {
            System.out.println("Server has accepted the operation.");
        }
    }

    public static byte GetAxis() {
        byte input;
        while (true) {
            System.out.println("Axis('X'/'Y'):");
            input = Getchar();
            if ((input == 'X') || (input == 'x'))
                return 'X';
            else if ((input == 'Y') || (input == 'y'))
                return 'Y';
            else
                System.out.println("Invalid axis");
        }
    }

    public static byte GetVal() {
        int input;
        while (true) {
            System.out.printf("Value(0~255):");
            input = sc.nextInt();
            if ((input >= 0) && (input <= 255))
                return (byte) input;
            else
                System.out.println("Invalid value");
        }
    }

    public static byte GetStat() {
        byte input;
        while (true) {
            System.out.println("'P' - Positive, 'N' - Negative, 'S' - Stop");
            System.out.printf("Direction('P'/'N'/'S'):");
            input = Getchar();
            switch (input) {
                case 'P':
                case 'p':
                    return CommModule.SERVO_MOVE_DIRECTION_POSE;
                case 'N':
                case 'n':
                    return CommModule.SERVO_MOVE_DIRECTION_NEGA;
                case 'S':
                case 's':
                    return CommModule.SERVO_MOVE_DIRECTION_STOP;
                default:
                    System.out.println("Invalid direction");
                    break;
            }
        }
    }

    public static byte Getchar() {
        while (true) {
            String input = sc.nextLine();
            byte rtnVal;
            try {
                rtnVal = (byte) input.charAt(0);
                return rtnVal;
            } catch (Exception ex) {
                System.out.println("Illegal input, input again.");
            }
        }
    }
}
