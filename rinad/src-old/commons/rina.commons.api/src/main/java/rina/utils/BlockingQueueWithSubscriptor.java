package rina.utils;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import eu.irati.librina.IPCException;

/**
 * Represents a blocking queue with a certain capacity, identified by an Integer.
 * The queue has the option of specifying a subscriptor. The subscriptor will be called
 * every time an object of class T is written to the queue.
 * @author eduardgrasa
 *
 */
public class BlockingQueueWithSubscriptor<T> {
	
	/**
	 * The class that is subscribed to this queue and 
	 * will be informed every time there is data available
	 * to be read
	 */
	private QueueReadyToBeReadSubscriptor queueReadyToBeReadSubscriptor = null;
	
	/**
	 * The class that is subscribed to this queue and 
	 * will be informed every time the queue capacity transitions from 
	 * full to having some capacity
	 * to be read
	 */
	private QueueReadyToBeWrittenSubscriptor queueReadyToBeWrittenSubscriptor = null;
	
	/**
	 * The queue
	 */
	private BlockingQueue<T> dataQueue = null;
	
	/**
	 * The id of the queue
	 */
	private int queueId = 0;
	
	/**
	 * true if it is an input queue, false if it is an output queue
	 */
	private boolean inputOutput = false;
	
	/**
	 * Constructs the queue with a queueId and a capacity
	 * @param queueId the identifier of the queue
	 * @param queueCapacity the capacity of the queue, it it is <= 0 an unlimited capacity queue will be used
	 * @param inputOutput true if it is an input queue, false if it is an output queue
	 */
	public BlockingQueueWithSubscriptor(int queueId, int queueCapacity, boolean inputOutput){
		this.queueId = queueId;
		if (queueCapacity <= 0){
			this.dataQueue = new LinkedBlockingQueue<T>();
		}else{
			this.dataQueue = new ArrayBlockingQueue<T>(queueCapacity);
		}
		this.inputOutput = inputOutput;
	}
	
	public void subscribeToQueueReadyToBeReadEvents(QueueReadyToBeReadSubscriptor queueReadyToBeReadSubscriptor){
		this.queueReadyToBeReadSubscriptor = queueReadyToBeReadSubscriptor;
	}
	
	public void subscribeToQueueReadyToBeWrittenEvents(QueueReadyToBeWrittenSubscriptor queueReadyToBeWrittenSubscriptor){
		this.queueReadyToBeWrittenSubscriptor = queueReadyToBeWrittenSubscriptor;
	}
	
	public BlockingQueue<T> getDataQueue(){
		return dataQueue;
	}
	
	public boolean isFull(){
		return this.dataQueue.remainingCapacity() <= 0;
	}
	
	/**
	 * Write the data byte array to the queue. The 
	 * queue subscriptor will be notified of new data available
	 * @param identifier
	 * @param data
	 * @throws IPCException
	 */
	public void writeDataToQueue(T data) throws IPCException{
		try{
			dataQueue.put(data);
			if (this.queueReadyToBeReadSubscriptor != null){
				this.queueReadyToBeReadSubscriptor.queueReadyToBeRead(this.queueId, this.inputOutput);
			}
		}catch(Exception ex){
			throw new IPCException("Problems writing data to queue "+this.queueId+". "+ex.getMessage());
		}
	}

	/**
	 * Returns the data in the queue. The thread calling this operation
	 * will block until data is available in the queue
	 * @return
	 * @throws InterruptedException
	 */
	public T take() throws IPCException{
		try{
			T data = this.dataQueue.take();
			if (this.dataQueue.remainingCapacity() == 1 && this.queueReadyToBeWrittenSubscriptor != null){
				this.queueReadyToBeWrittenSubscriptor.queueReadyToBeWritten(this.queueId, this.inputOutput);
			}
			return data;
		}catch(Exception ex){
			throw new IPCException("Problems reading data from queue "+this.queueId+". "+ex.getMessage());
		}
	}
}
