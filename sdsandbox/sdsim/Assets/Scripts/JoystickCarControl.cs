using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityStandardAssets.CrossPlatformInput;

public class JoystickCarControl : MonoBehaviour 
{
	public GameObject carObj;
	private ICar car;
	//public float h;
	//public float v;
	public char nowKey = 'w';

	public float MaximumSteerAngle = 16.0f; //has to be kept in sync with the car, as that's a private var.
	
	void Awake()
	{
		if(carObj != null)
			car = carObj.GetComponent<ICar>();
	}

	private void FixedUpdate()
	{
		float h = CrossPlatformInputManager.GetAxis("Horizontal");
		float v = CrossPlatformInputManager.GetAxis("Vertical");
		float handbrake = CrossPlatformInputManager.GetAxis("Jump");
		car.RequestSteering(h * MaximumSteerAngle);
		car.RequestThrottle(v);
		//car.RequestFootBrake(v);
		car.RequestHandBrake(handbrake);

//		if(Input.GetKeyDown(KeyCode.W)){
//			nowKey = 'w';
//			car.RequestCommand(nowKey);
//		}else if(Input.GetKeyDown(KeyCode.A)){
//			nowKey = 'a';
//			car.RequestCommand(nowKey);
//		}else if(Input.GetKeyDown(KeyCode.D)){
//			nowKey = 'd';
//			car.RequestCommand(nowKey);
//		}else if(Input.GetKeyDown(KeyCode.S)){
//			nowKey = 's';
//			car.RequestCommand(nowKey);
//		}else{
//				car.RequestCommand(nowKey);
//		}
	}
}
