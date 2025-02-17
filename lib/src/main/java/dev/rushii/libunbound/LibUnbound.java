package dev.rushii.libunbound;

import java.util.Objects;

/**
 * JNI interface to the native side of this library.
 */
@SuppressWarnings("unused")
public class LibUnbound {
	private static int cachedBytecodeVersion = -1;

	static {
		System.loadLibrary("unbound");
	}

	/**
	 * Obtains the Hermes Bytecode (HBC) version that the Hermes runtime supports.
	 * This method can only be called after the Hermes native lib has been loaded into the process.
	 *
	 * @throws IllegalStateException If Hermes has not yet been loaded.
	 * @throws RuntimeException      If failing to find the required native symbol.
	 */
	public static int getHermesRuntimeBytecodeVersion() {
		if (cachedBytecodeVersion > 0)
			return cachedBytecodeVersion;

		return (cachedBytecodeVersion = (int) getHermesRuntimeBytecodeVersion0());
	}

	/**
	 * Quick and potentially inconclusive check to determine whether some bytes are valid hermes bytecode.
	 *
	 * @param bytes Nonnull byte array.
	 */
	public static boolean isHermesBytecode(byte[] bytes) {
		return isHermesBytecode0(Objects.requireNonNull(bytes));
	}

	private static native long getHermesRuntimeBytecodeVersion0();

	private static native boolean isHermesBytecode0(byte[] bytes);
}
