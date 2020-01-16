using UnityEngine;
using System.Collections;
using UnityEngine.UI;
public class InputMaxFrames : MonoBehaviour{
    public Logger logger;
    void Start () {

    transform.GetComponent<InputField>().onEndEdit.AddListener(End_Value);

}

public void  End_Value(string inp){
    logger.maxFramesToLog = int.Parse(inp);
}
}