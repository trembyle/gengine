//
// GameCamera.cpp
//
// Clark Kromenaker
//
#include "GameCamera.h"

#include "AudioListener.h"
#include "Camera.h"
#include "Collisions.h"
#include "GEngine.h"
#include "Scene.h"
#include "Sphere.h"
#include "Triangle.h"
#include "UICanvas.h"

GameCamera::GameCamera()
{
    mCamera = AddComponent<Camera>();
    AddComponent<AudioListener>();
}

void GameCamera::SetAngle(const Vector2& angle)
{
	SetAngle(angle.GetX(), angle.GetY());
}

void GameCamera::SetAngle(float yaw, float pitch)
{
	SetRotation(Quaternion(Vector3::UnitY, yaw) * Quaternion(Vector3::UnitX, pitch));
}

void GameCamera::OnUpdate(float deltaTime)
{
	// We don't move/turn unless some input causes it.
	float forwardSpeed = 0.0f;
	float strafeSpeed = 0.0f;
	float turnSpeed = 0.0f;
	float pitchSpeed = 0.0f;
	float verticalSpeed = 0.0f;
	
	// Pan/Pan modifiers are activated with CTRL/SHIFT keys.
	// These work EVEN IF text input is active.
	bool panModifierActive = Services::GetInput()->IsKeyPressed(SDL_SCANCODE_LCTRL) ||
							 Services::GetInput()->IsKeyPressed(SDL_SCANCODE_RCTRL);;
	bool pitchModifierActive = Services::GetInput()->IsKeyPressed(SDL_SCANCODE_LSHIFT) ||
							   Services::GetInput()->IsKeyPressed(SDL_SCANCODE_RSHIFT);
	
	// W/S/A/D and Up/Down/Left/Right only work if text input isn't stealing input.
	if(!Services::GetInput()->IsTextInput())
	{
		if(Services::GetInput()->IsKeyPressed(SDL_SCANCODE_W) ||
		   Services::GetInput()->IsKeyPressed(SDL_SCANCODE_UP))
		{
			if(pitchModifierActive)
			{
				pitchSpeed += kRotationSpeed;
			}
			else if(panModifierActive)
			{
				verticalSpeed += kSpeed;
			}
			else
			{
				forwardSpeed += kSpeed;
			}
		}
		else if(Services::GetInput()->IsKeyPressed(SDL_SCANCODE_S) ||
				Services::GetInput()->IsKeyPressed(SDL_SCANCODE_DOWN))
		{
			if(pitchModifierActive)
			{
				pitchSpeed -= kRotationSpeed;
			}
			else if(panModifierActive)
			{
				verticalSpeed -= kSpeed;
			}
			else
			{
				forwardSpeed -= kSpeed;
			}
		}
		
		if(Services::GetInput()->IsKeyPressed(SDL_SCANCODE_D) ||
		   Services::GetInput()->IsKeyPressed(SDL_SCANCODE_RIGHT))
		{
			if(pitchModifierActive)
			{
				turnSpeed += kRotationSpeed;
			}
			else if(panModifierActive)
			{
				strafeSpeed += kSpeed;
			}
			else
			{
				turnSpeed += kRotationSpeed;
			}
		}
		if(Services::GetInput()->IsKeyPressed(SDL_SCANCODE_A) ||
		   Services::GetInput()->IsKeyPressed(SDL_SCANCODE_LEFT))
		{
			if(pitchModifierActive)
			{
				turnSpeed -= kRotationSpeed;
			}
			else if(panModifierActive)
			{
				strafeSpeed -= kSpeed;
			}
			else
			{
				turnSpeed -= kRotationSpeed;
			}
		}
	}
	
	// If left mouse button is held down, mouse movement contributes to camera movement.
	bool leftMousePressed = Services::GetInput()->IsMouseButtonPressed(InputManager::MouseButton::Left);
	if(leftMousePressed)
	{
		// Pan modifier also activates if right mouse button is pressed.
		panModifierActive |= Services::GetInput()->IsMouseButtonPressed(InputManager::MouseButton::Right);
		
		// Mouse delta is in pixels.
		// We normalize this by estimating "max pixel movement" per frame.
		Vector2 mouseDelta = Services::GetInput()->GetMouseDelta();
		mouseDelta /= kMouseRangePixels;
		
		// Lock the mouse!
		if(mouseDelta.GetLengthSq() > 0.0f)
		{
			Services::GetInput()->LockMouse();
		}
		
		// Mouse y-axis affect depends on modifiers.
		if(pitchModifierActive)
		{
			pitchSpeed += mouseDelta.y * kRotationSpeed;
		}
		else if(panModifierActive)
		{
			verticalSpeed += mouseDelta.y * kSpeed;
		}
		else
		{
			forwardSpeed += mouseDelta.y * kSpeed;
		}
		
		// Mouse x-axis affect also depends on modifiers.
		// Note that pitch modifier and no modifier do the same thing!
		if(pitchModifierActive)
		{
			turnSpeed += mouseDelta.x * kRotationSpeed;
		}
		else if(panModifierActive)
		{
			strafeSpeed += mouseDelta.x * kSpeed;
		}
		else
		{
			turnSpeed += mouseDelta.x * kRotationSpeed;
		}
	}
	
	// Alt keys just increase all speeds!
	if(Services::GetInput()->IsKeyPressed(SDL_SCANCODE_LALT) ||
	   Services::GetInput()->IsKeyPressed(SDL_SCANCODE_RALT))
	{
		forwardSpeed *= kFastSpeedMultiplier;
		strafeSpeed *= kFastSpeedMultiplier;
		turnSpeed *= kFastSpeedMultiplier;
		pitchSpeed *= kFastSpeedMultiplier;
		verticalSpeed *= kFastSpeedMultiplier;
	}
	
	// For forward movement, we want to disregard any y-facing; just move on X/Z plane.
	Vector3 forward = GetForward();
	forward.y = 0.0f;
	forward.Normalize();
	
	// Calculate desired position based on all speeds.
	// We may not actually move to this exact position, due to collision.
	Vector3 position = GetPosition();
	position += forward * forwardSpeed * deltaTime;
	position += GetRight() * strafeSpeed * deltaTime;
	
	// Calculate new desired height and apply that to position y.
	float height = mHeight + verticalSpeed * deltaTime;
	float floorY = GEngine::inst->GetScene()->GetFloorY(position);
	position.SetY(floorY + height);
	
	// Perform collision checks and resolutions.
	ResolveCollisions(position);
	
	// Set position after resolving collisions.
	GetTransform()->SetPosition(position);
	
	// Height may also be affected by collision. After resolving,
	// we can see if our height changed and save it.
	float newFloorY = GEngine::inst->GetScene()->GetFloorY(position);
	float heightForReal = position.GetY() - newFloorY;
	mHeight = heightForReal;
	
	// Apply turn movement.
	GetTransform()->Rotate(Vector3::UnitY, turnSpeed * deltaTime, Transform::Space::World);
	
	// Apply pitch movement.
	GetTransform()->Rotate(GetRight(), -pitchSpeed * deltaTime, Transform::Space::World);
	
	// Raycast to the ground and always maintain a desired height.
	//TODO: This does not apply when camera boundaries are disabled!
	//TODO: Doesn't apply when middle mouse button is held???
	Scene* scene = GEngine::inst->GetScene();
	if(scene != nullptr)
	{
		Vector3 pos = GetPosition();
		floorY = scene->GetFloorY(pos);
		pos.SetY(floorY + mHeight);
		SetPosition(pos);
	}
	
	// For debugging, output camera position on key press.
	if(Services::GetInput()->IsKeyDown(SDL_SCANCODE_P))
	{
		std::cout << GetPosition() << std::endl;
	}
	
	// Handle hovering and clicking on scene objects.
	//TODO: Original game seems to ONLY check this when the mouse cursor moves or is clicked (in other words, on input).
	//TODO: Maybe we should do that too?
	if(mCamera != nullptr && !Services::GetInput()->MouseLocked())
	{
		// Only allow scene interaction if pointer isn't over a UI widget.
		if(!UICanvas::DidWidgetEatInput())
		{
			// Calculate mouse click ray.
			Vector2 mousePos = Services::GetInput()->GetMousePosition();
			Vector3 worldPos = mCamera->ScreenToWorldPoint(mousePos, 0.0f);
			Vector3 worldPos2 = mCamera->ScreenToWorldPoint(mousePos, 1.0f);
			Vector3 dir = (worldPos2 - worldPos).Normalize();
			Ray ray(worldPos, dir);
		
			// If we can interact with whatever we are pointing at, highlight the cursor.
			bool canInteract = GEngine::inst->GetScene()->CheckInteract(ray);
			if(canInteract)
			{
				GEngine::inst->UseHighlightCursor();
			}
			else
			{
				GEngine::inst->UseDefaultCursor();
			}
			
			// If left mouse button is released, try to interact with whatever it is over.
			// Need to do this, even if canInteract==false, because floor can be clicked to move around.
			if(Services::GetInput()->IsMouseButtonUp(InputManager::MouseButton::Left))
			{
				GEngine::inst->GetScene()->Interact(ray);
			}
		}
	}
	
	// Clear camera lock if left mouse is not pressed.
	// Do this AFTER interact check to avoid interacting with things when exiting mouse locked movement mode.
	if(!leftMousePressed)
	{
		Services::GetInput()->UnlockMouse();
	}
}

void GameCamera::ResolveCollisions(Vector3& position)
{
	// No bounds model = no collision.
	if(mBoundsModel == nullptr) { return; }
	
	// We'll represent the camera with a sphere and the bounds are a model (triangles).
	// Iterate and do a collision check against the triangles of the bounds model.
	auto meshes = mBoundsModel->GetMeshes();
	for(auto& mesh : meshes)
	{
		// Bounds model is positioned at (0,0,0) in world space (so no need to multiply local to world...it's identity).
		// BUT each mesh in the model has its own local coordinate system!
		// We need to convert camera position to local space of the mesh before doing collision check.
		//TODO: Inverse operation here is expensive - can probably be more efficient somehow...
		Matrix4 meshToLocal = mesh->GetLocalTransformMatrix();
		Matrix4 localToMesh = meshToLocal.Inverse();
		Vector3 meshPosition = localToMesh.TransformPoint(position);
		
		// Create sphere at position.
		const float kCameraColliderRadius = 25.0f;
		Sphere s(meshPosition, kCameraColliderRadius);
		
		// Iterate submeshes/submesh triangles.
		auto submeshes = mesh->GetSubmeshes();
		for(auto& submesh : submeshes)
		{
			Vector3 p0, p1, p2;
			int triangleCount = submesh->GetTriangleCount();
			for(int i = 0; i < triangleCount; i++)
			{
				if(submesh->GetTriangle(i, p0, p1, p2))
				{
					// If an intersection exists, resolve it by "pushing" mesh position out.
					Vector3 intersection;
					if(Collisions::TestSphereTriangle(s, Triangle(p0, p1, p2), intersection))
					{
						meshPosition += intersection;
						s = Sphere(meshPosition, kCameraColliderRadius);
					}
				}
			}
		}
		
		// We modified the local position while iterating submeshes.
		// We now need to go back to "world" space.
		position = meshToLocal.TransformPoint(meshPosition);
	}
}
