import java.util.Scanner;

public class ex1 {
    public static void main (String argv[]) {
	System.out.printf ("Here is a java example%n");

	if (argv.length == 0) {
	    System.out.printf("(no args provided)%n");
	} else {
	    int i;
	    for (i = 0; i < argv.length; i++) {
		System.out.printf("argv[%d] is '%s'%n",i, argv[i]);
	    }
	}
	
 	Scanner sc = new Scanner (System.in);
	System.out.printf ("What is your name? ");
	String name = sc.nextLine ();
	System.out.printf ("Well, hello %s, what a lovely name\n", name);
    }
}
