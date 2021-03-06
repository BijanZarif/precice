//
// ASCoDT - Advanced Scientific Computing Development Toolkit
//
// This file was generated by ASCoDT's simplified SIDL compiler.
//
// Authors: Tobias Weinzierl, Atanas Atanasov   
//
package fsi;


public class FSICommNative2NativePlainPort extends FSICommAbstractPort {
  private long _ref;
  public FSICommNative2NativePlainPort() {
    super();
    createInstance();
  }
  /**
   * proxy for the native factory method
   */  
  public native void createInstance();
  
  /**
   * frees the memory of the component
   */
  public native void destroyInstance(long ref);
  
  
  /**
   * Connect a uses port.
   *
   * @throw If already connected to another port.
   * @see Operation with name of any implementing component 
   */
  @Override
  public void connect(FSIComm  port) throws de.tum.ascodt.utils.exceptions.ASCoDTException {
    super.connect(port);
    connect(_ref,_destination.getReference());
  }
  
  public native void connect(long ref,long cca_application);
   
  /**
   * Disconnect a port.
   * 
   * @throw If already not connected to type port.
   * @see Operation with name of any implementing component 
   */
  public void disconnect(FSIComm  port) throws de.tum.ascodt.utils.exceptions.ASCoDTException {
     super.disconnect(port);
  }
  
  public void destroy(){
     destroyInstance(_ref);
  }
  
  public long getReference(){
    return _ref;
  }
  
  public void setReference(long ref){
     _ref=ref;
  }

  public void transferCoordinates(final int coordId[],final int offsets[],final String hosts[]) {
    //DO NOTHING HERE
  }
  
   public void transferCoordinatesParallel(final int coordId[],final int offsets[],final String hosts[]) {
    //DO NOTHING HERE
  }
  

  public void startDataTransfer() {
    //DO NOTHING HERE
  }
  
   public void startDataTransferParallel() {
    //DO NOTHING HERE
  }
  

  public void endDataTransfer(int ack[]) {
    //DO NOTHING HERE
  }
  
   public void endDataTransferParallel(int ack[]) {
    //DO NOTHING HERE
  }
  
  

}
 


