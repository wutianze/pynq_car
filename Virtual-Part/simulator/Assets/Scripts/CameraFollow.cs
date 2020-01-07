using UnityEngine;
using System.Collections;

public class CameraFollow : MonoBehaviour {

	public bool ifFollow = false;
	public Transform target;
	public float approachPosRate = 0.02f;
	public float approachRotRate = 0.02f;
	// Update is called once per frame
	public float sensitivityMouse = 2f;
	public float scrollSpeed = 10f;
	void FixedUpdate () 
	{
		if(ifFollow){
transform.position = Vector3.Lerp(transform.position, target.position, approachPosRate);
		transform.rotation = Quaternion.Lerp(transform.rotation, target.rotation, approachRotRate);
		}
		else{
        if (Input.GetMouseButton(1))
        {
            transform.Rotate(0, Input.GetAxis("Mouse X") * sensitivityMouse, 0,Space.World);
        }
		float upDown= Input.GetAxis("Mouse ScrollWheel") * scrollSpeed;
		transform.Translate(new Vector3(0,upDown,0),Space.World);
		if(Input.GetKey(KeyCode.UpArrow))
		{
			transform.Translate(new Vector3(0,0,20*Time.deltaTime));
		}
		if(Input.GetKey(KeyCode.DownArrow))
		{
			transform.Translate(new Vector3(0,0,-20*Time.deltaTime));
		}
		if(Input.GetKey(KeyCode.LeftArrow))
		{
			transform.Translate(new Vector3(-20*Time.deltaTime,0,0));
		}
		if(Input.GetKey(KeyCode.RightArrow))
		{
			transform.Translate(new Vector3(20*Time.deltaTime,0,0));
		}
		}
	}
}
