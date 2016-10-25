#pragma once
#include <cstdint>
#include <vector>
#include "ion/text/binpacker.h"
#include "ion/math/matrix.h"
#include "ion/math/angle.h"
#include "ion/math/rotation.h"
#include "IonFwd.h"
#include "ion/math/range.h"

class Camera
{
public:
   Camera(ion::gfx::StateTablePtr rootStateTable);
   ~Camera();

   void UpdateUniforms();

   const ion::gfx::UniformBlockPtr & GetViewportUniforms() const;
   const ion::gfx::UniformBlockPtr & Get3dUniforms() const;

   /**
   The position of the camera.
   */
   const ion::math::Point3d & PositionWorld() const;
   void SetPositionWorld(const ion::math::Point3d & positionWorld);
   void OffsetPositionWorld(const ion::math::Vector3d & offsetWorld);

   //const ion::math::Vector4d & GetOrthoBounds() const;
   //void SetOrthoBounds(const ion::math::Vector4d & orthoBounds);

   /**
   The vertical viewing angle of the camera, in degrees.

   Determines how "wide" the view of the camera is. Large angles appear to be zoomed out,
   as the camera has a wide view. Small values appear to be zoomed in, as the camera has a
   very narrow view.

   The value must be between 0 and 180.
   */
   ion::math::Angled FieldOfView() const;
   void SetFieldOfView(const ion::math::Angled fieldOfView);

   double AspectRatio() const;
   void SetAspectRatio(const double aspect);
   void SetUseAutoAspectRatio();


   /**
   The closest visible distance from the camera.

   Objects that are closer to the camera than the near plane distance will not be visible.

   Value must be greater than 0.
   */
   double NearPlane() const;

   /**
   The farthest visible distance from the camera.

   Objects that are further away from the than the far plane distance will not be visible.

   Value must be greater than the near plane
   */
   double FarPlane() const;

   /**
   Sets the near and far plane distances.

   Everything between the near plane and the var plane will be visible. Everything closer
   than the near plane, or farther than the far plane, will not be visible.

   @param nearPlane  Minimum visible distance from camera. Must be > 0
   @param farPlane   Maximum visible distance from vamera. Must be > nearPlane
   */
   void SetNearAndFarPlanes(const double nearPlane, const double farPlane);

   /**
   A rotation matrix that determines the direction the camera is looking.

   Does not include translation (the camera's position).
   */
   const ion::math::Rotationd & Orientation() const;
   void SetOrientation(const ion::math::Rotationd & orient);

   const ion::math::Vector3d & Scale() const;
   void SetScale(const ion::math::Vector3d & scale);

   /**
   Orients the camera so that is it directly facing `position`

   @param position  the position to look at
   */
   void LookAt(const ion::math::Point3d & center, const ion::math::Vector3d & up);
   void LookAt(const ion::math::Point3d & eye, const ion::math::Point3d & center, const ion::math::Vector3d & up);

   //Methods to set the projection matrix
   void Perspective(const ion::math::Angled fov, const double aspect, const double nearPlane, const double farPlane);
   void Perspective(const ion::math::Angled fov, const double nearPlane, const double farPlane);

   //void Ortho(const ion::math::Vector4ui & bounds);
   //void Ortho(const ion::math::Vector4ui & bounds, const double nearVal, const double farVal);

   bool UsingPerspectiveProj() const;

   /**
   The width divided by the height of the screen/window/viewport

   Incorrect values will make the 3D scene look stretched.
   */
   double ViewportAspectRatio() const;
   void SetViewportBounds(const ion::math::Vector2ui & bounds);
   void SetViewportBounds(const ion::math::Range2ui & bounds);
   const ion::math::Range2ui & GetViewportBounds() const; //X, Y, Width, Height

                                                 //Viewpoint helper functions
   ion::math::Point2ui GetViewportPosition() const;
   ion::math::Vector2ui GetViewportSize() const;
   uint32_t GetViewportWidth() const;
   uint32_t GetViewportHeight() const;
   uint32_t GetViewportX() const;
   uint32_t GetViewportY() const;

   /** A unit vector representing the direction the camera is facing */
   const ion::math::Vector3d & Forward() const;

   /** A unit vector representing the direction to the right of the camera*/
   const ion::math::Vector3d & Right() const;

   /** A unit vector representing the direction out of the top of the camera*/
   const ion::math::Vector3d & Up() const;

   /**
   The combined camera transformation matrix, including perspective projection.

   This is the complete matrix to use in the vertex shader.
   */
   const ion::math::Matrix4d & ViewProjectionMatrix() const;

   /**
   The perspective projection transformation matrix
   */
   const ion::math::Matrix4d & ProjectionMatrix() const;

   ///**
   //The translation and rotation matrix of the camera.

   //Same as the `matrix` method, except the return value does not include the projection
   //transformation.
   //*/
   const ion::math::Matrix4d & ViewMatrix() const;

   //ion::math::Point3d Project(const ion::math::Point3d & worldPosition, const ion::math::Matrix4d & modelMatrix) const;
   //static ion::math::Point3d Project(const ion::math::Point3d & obj,
   //                          const ion::math::Matrix4d & model,
   //                          const ion::math::Matrix4d & proj,
   //                          const glm::uvec4 & viewport,
   //                          bool isPerspective,
   //                          bool isUsingNegativeDepthBuffer);
private:

   ion::math::Range2ui m_WindowBounds; //X, Y, Width, Height

   //View Parameters
   ion::math::Vector3d m_Scale;
   ion::math::Point3d m_PositionWorld;
   ion::math::Rotationd  m_Orientation;

   //Projection parameters
   bool m_PerspectiveProjection; //true for perspective, false for ortho
   double m_NearPlane;
   double m_FarPlane;

   bool m_AutoAspectRatio;
   double m_AspectRatio;

   //Perspective params
   ion::math::Angled m_FieldOfView;

   //Ortho params
   ion::math::Vector4d m_OrthoBounds;

   //Variables to store cache since view will change once per frame but possibly used many times
   mutable bool c_ViewProjectionMatrixDirty;
   mutable ion::math::Matrix4d c_ViewProjectionMatrix;

   mutable bool c_ProjectionMatrixDirty;
   mutable ion::math::Matrix4d c_ProjectionMatrix;

   mutable bool c_ViewMatrixDirty;
   mutable ion::math::Matrix4d c_ViewMatrix;

   ion::gfx::StateTablePtr m_RootStateTable;
   ion::gfx::UniformBlockPtr m_ViewportUniforms;
   ion::gfx::UniformBlockPtr m_3dUniforms;


};

