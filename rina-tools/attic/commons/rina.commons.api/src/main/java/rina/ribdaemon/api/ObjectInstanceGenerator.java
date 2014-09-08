package rina.ribdaemon.api;

/**
 * Generates the object instances identifiers
 * @author eduardgrasa
 *
 */
public class ObjectInstanceGenerator {
	public static long getObjectInstance(){
		return System.nanoTime();
	}
}
