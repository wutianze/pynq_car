using UnityEngine;
using System.Collections;
using UnityEngine.UI;

//[RequireComponent(typeof(tk.JsonTcpClient))]
public class InputIp : MonoBehaviour{
    //public GameObject NetworkSteering;
    public GameObject NetworkSteering;
        void Start () {

    transform.GetComponent<InputField>().onEndEdit.AddListener(End_Value);

}

public void  End_Value(string inp){
    NetworkSteering.GetComponent<tk.JsonTcpClient>().SetIp(inp);
}
}