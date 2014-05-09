package rina.ribdaemon.api;

/**
 * Defines create/delete/read/write/start/stop permissions 
 * for a RIB node
 * @author eduardgrasa
 *
 */
public class CDAPOperationsPermission {
	private boolean create = true;
	private boolean delete = true;
	private boolean read = true;
	private boolean write = true;
	private boolean start = true;
	private boolean stop = true;
	
	public CDAPOperationsPermission(boolean create, boolean delete, boolean read, boolean write, boolean start, boolean stop){
		this.create = create;
		this.delete = delete;
		this.read = read;
		this.write = write;
		this.start = start;
		this.stop = stop;
	}

	public boolean isCreate() {
		return create;
	}

	public void setCreate(boolean create) {
		this.create = create;
	}

	public boolean isDelete() {
		return delete;
	}

	public void setDelete(boolean delete) {
		this.delete = delete;
	}

	public boolean isRead() {
		return read;
	}

	public void setRead(boolean read) {
		this.read = read;
	}

	public boolean isWrite() {
		return write;
	}

	public void setWrite(boolean write) {
		this.write = write;
	}

	public boolean isStart() {
		return start;
	}

	public void setStart(boolean start) {
		this.start = start;
	}

	public boolean isStop() {
		return stop;
	}

	public void setStop(boolean stop) {
		this.stop = stop;
	}
}
