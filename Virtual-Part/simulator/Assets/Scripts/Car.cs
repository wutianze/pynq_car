using UnityEngine;
using System.Collections;


public class Car : MonoBehaviour, ICar {

	public WheelCollider[] wheelColliders;
	public Transform[] wheelMeshes;

	public float maxTorque = 50f;
	public float maxSpeed = 6f;

	public Transform centrOfMass;

	public float requestTorque = 0f;
	public float requestBrake = 0f;
	public float requestSteering = 0f;
	public float requestSpeed = 0f;
	private bool useSpeed = false;

	public Vector3 acceleration = Vector3.zero;
	public Vector3 prevVel = Vector3.zero;

	public Vector3 startPos;
	public Quaternion startRot;

	public float length = 1.7f;

	Rigidbody rb;

	//for logging
	public float lastSteer = 0.0f;
	public float lastAccel = 0.0f;

	//when the car is doing multiple things, we sometimes want to sort out parts of the training
	//use this label to pull partial training samples from a run 
	public string activity = "keep_lane";

    public float maxSteer = 16.0f;

	//name of the last object we hit.
	public string last_collision = "none";
	
	// Use this for initialization
	void Awake () 
	{
		rb = GetComponent<Rigidbody>();

		if(rb && centrOfMass)
		{
			rb.centerOfMass = centrOfMass.localPosition;
		}

		requestTorque = 0f;
		requestSteering = 0f;

		SavePosRot();

        maxSteer = PlayerPrefs.GetFloat("max_steer", 16.0f);       
	}

	public void SavePosRot()
	{
		startPos = transform.position;
		startRot = transform.rotation;
	}

	public void RestorePosRot()
	{
		Set(startPos, startRot);
	}

	public char comNow = 'w';
	//Pynq command manager
	public void RequestCommand(char c){
		comNow = c;
		switch(c){
			case 'w':
				requestSteering = 0.0f;
				requestTorque = 0.2f;
				requestBrake = 0f;
				break;
			case 'a':
				requestSteering = -7.0f;
				requestTorque = 0.2f;
				requestBrake = 0f;
				break;
			case 'd':
				requestSteering = 7.0f;
				requestTorque = 0.2f;
				requestBrake = 0f;
				break;
			case 's':
				requestSteering = 0.0f;
				requestTorque = 0.0f;
				requestBrake = 1.0f;
				break;
			}
	}

	public char GetComNow(){
		return comNow;
	}

	public void RequestThrottle(float val)
	{
		requestTorque = val;
		requestBrake = 0f;
		useSpeed = false;
		//Debug.Log("request throttle: " + val);
	}

    public void SetMaxSteering(float val)
    {
        maxSteer = val;

        PlayerPrefs.SetFloat("max_steer", maxSteer);
        PlayerPrefs.Save();
    }

    public float GetMaxSteering()
    {
        return maxSteer;
    }

	public void RequestSteering(float val)
	{
		requestSteering = Mathf.Clamp(val, -maxSteer, maxSteer);
		//Debug.Log("request steering: " + val);
	}

	public void RequestSpeed(float val){
		requestSpeed = val;
		requestBrake = 0f;
		useSpeed = true;
		//RequestThrottle(val - rb.velocity.magnitude/maxSpeed);
	}

	public void Set(Vector3 pos, Quaternion rot)
	{
		rb.position = pos;
		rb.rotation = rot;

		//just setting it once doesn't seem to work. Try setting it multiple times..
		StartCoroutine(KeepSetting(pos, rot, 1));
	}

	IEnumerator KeepSetting(Vector3 pos, Quaternion rot, int numIter)
	{
		while(numIter > 0)
		{
			rb.isKinematic = true;
			
			yield return new WaitForFixedUpdate();

			rb.position = pos;
			rb.rotation = rot;
			transform.position = pos;
			transform.rotation = rot;

			numIter--;

			rb.isKinematic = false;
		}
	}

	public float GetSteering()
	{
		return requestSteering;
	}

	public float GetThrottle()
	{
		return requestTorque;
	}
	public float GetMaxSpeed(){
		return maxSpeed;
	}
	public float GetFootBrake()
	{
		return requestBrake;
	}

	public float GetHandBrake()
	{
		return 0.0f;
	}

	public Vector3 GetVelocity()
	{
		return rb.velocity;
	}

	public Vector3 GetAccel()
	{
		return acceleration;
	}

	public float GetOrient ()
	{
		Vector3 dir = transform.forward;
		return Mathf.Atan2( dir.z, dir.x);
	}

	public Transform GetTransform()
	{
		return this.transform;
	}

	public bool IsStill()
	{
		return rb.IsSleeping();
	}

	public void RequestFootBrake(float val)
	{
		requestBrake = val;
	}

	public void RequestHandBrake(float val)
	{
		//todo
	}
	
	// Update is called once per frame
	void Update () {
	
		UpdateWheelPositions();
	}

	public string GetActivity()
	{
		return activity;
	}

	public void SetActivity(string act)
	{
		activity = act;
	}

	void FixedUpdate()
	{
		lastSteer = requestSteering;
		lastAccel = requestTorque;

		float throttle = requestTorque * maxTorque;
		float steerAngle = requestSteering;
		float speed = requestSpeed;
        float brake = requestBrake;


		//front two tires.
		wheelColliders[2].steerAngle = steerAngle;
		wheelColliders[3].steerAngle = steerAngle;

		//four wheel drive at the moment
		foreach(WheelCollider wc in wheelColliders)
		{
			if(rb.velocity.magnitude >= maxSpeed)
			{
				wc.motorTorque = -0.15f;
			}
			else
			{
				//Debug.Log("now speed:"+rb.velocity.magnitude);
				//Debug.Log("request speed:"+speed);
				if(useSpeed){
					throttle = ((speed - rb.velocity.magnitude)/maxSpeed)*maxTorque;
				}
				//Debug.Log("request throttle:"+throttle);
				wc.motorTorque = throttle;
			}

			// most time zero
			wc.brakeTorque = 400f * brake;
		}

		acceleration = rb.velocity - prevVel;
	}

	void FlipUpright()
	{
		Quaternion rot = Quaternion.Euler(180f, 0f, 0f);
		this.transform.rotation = transform.rotation * rot;
		transform.position = transform.position + Vector3.up * 2;
	}

	void UpdateWheelPositions()
	{
		Quaternion rot;
		Vector3 pos;

		for(int i = 0; i < wheelColliders.Length; i++)
		{
			WheelCollider wc = wheelColliders[i];
			Transform tm = wheelMeshes[i];

			wc.GetWorldPose(out pos, out rot);

			tm.position = pos;
			tm.rotation = rot;
		}
	}

	//get the name of the last object we collided with
	public string GetLastCollision()
	{
		return last_collision;
	}

	public void ClearLastCollision()
	{
		last_collision = "none";
	}

	void OnCollisionEnter(Collision col)
	{
		last_collision = col.gameObject.name;
	}
}
