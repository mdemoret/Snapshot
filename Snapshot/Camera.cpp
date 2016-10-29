#include "Camera.hpp"

#include <cmath>
#include "ion/math/vectorutils.h"
#include "ion/math/fieldofview.h"
#include "ion/gfx/uniformblock.h"
#include "ion/gfx/shaderinputregistry.h"
#include "ion/gfx/statetable.h"
#include "Macros.h"
#include "ion/math/angleutils.h"

using namespace ion::gfx;
using namespace ion::math;

static const float MaxVerticalAngle = 85.0f; //must be less than 90 to avoid gimbal lock

Camera::Camera(StateTablePtr rootStateTable) :
   m_WindowBounds(Range2ui::BuildWithSize(Point2ui::Zero(), Vector2ui::Fill(100))),
   m_PerspectiveProjection(true),
   m_NearPlane(0.01),
   m_FarPlane(100.0),
   m_AutoAspectRatio(true),
   m_AspectRatio(1.0),
   m_FieldOfView(Angled::FromDegrees(45.0)),
   m_OrthoBounds(0.0, 0.0, 1.0, 1.0),
   c_ViewProjectionMatrixDirty(true),
   c_ViewProjectionMatrix(Matrix4d::Identity()),
   c_ProjectionMatrixDirty(true),
   c_ProjectionMatrix(Matrix4d::Identity()),
   c_ViewMatrixDirty(true),
   c_ViewMatrix(Matrix4d::Identity()),
   m_RootStateTable(rootStateTable),
   m_LookAtCenter(Point3d::Zero()),
   m_RA(Angled::FromDegrees(0.0)),
   m_DEC(Angled::FromDegrees(0.0)),
   m_Radius(Vector1d(4.0))
{
   //Build the uniform block that will always be set by the camera
   m_ViewportUniforms = UniformBlockPtr(new UniformBlock());
   m_3dUniforms = UniformBlockPtr(new UniformBlock());

   m_ViewportUniforms->AddUniform(ShaderInputRegistry::GetGlobalRegistry()->Create<Uniform>(UNIFORM_GLOBAL_VIEWPORTSIZE, Vector2i(100, 100)));
   m_3dUniforms->AddUniform(ShaderInputRegistry::GetGlobalRegistry()->Create<Uniform>(UNIFORM_GLOBAL_PROJECTIONMATRIX, Matrix4f::Identity()));
   m_3dUniforms->AddUniform(ShaderInputRegistry::GetGlobalRegistry()->Create<Uniform>(UNIFORM_GLOBAL_MODELVIEWMATRIX, Matrix4f::Identity()));
}

Camera::~Camera() {}

void Camera::UpdateUniforms() 
{
   m_ViewportUniforms->SetUniformByName<Vector2i>(UNIFORM_GLOBAL_VIEWPORTSIZE, Vector2i(GetViewportSize()));
   m_3dUniforms->SetUniformByName<Matrix4f>(UNIFORM_GLOBAL_PROJECTIONMATRIX, Matrix4f(ProjectionMatrix()));
   m_3dUniforms->SetUniformByName<Matrix4f>(UNIFORM_GLOBAL_MODELVIEWMATRIX, Matrix4f(ViewMatrix()));
}

const UniformBlockPtr& Camera::GetViewportUniforms() const
{
   return m_ViewportUniforms;
}

const UniformBlockPtr& Camera::Get3dUniforms() const
{
   return m_3dUniforms;
}

//void Camera::Reset()
//{
//   //Reset all the values which is useful when changing viewpoints
//   m_FieldOfView = 45.0;
//   m_AspectRatio = 1.0;
//   m_AutoAspectRatio = true;
//   m_NearPlane = 0.01;
//   m_FarPlane = 100.0;
//   m_PositionWorld = Point3d(0.0, 0.0, 0.0);
//   m_Orientation = Matrix3d::Identity();
//   m_Scale = Point3d(1.0);
//   c_ViewMatrixDirty = true;
//   c_ViewProjectionMatrixDirty = true;
//   m_WindowBounds = Vector4ui(0, 0, 100, 100);
//   m_PerspectiveProjection = true;
//   c_ProjectionMatrix = Matrix4d::Identity();
//   c_ProjectionMatrixDirty = true;
//}

Point3d Camera::PositionWorld() const
{
   return m_Radius[0] * Point3d(Cosine(m_RA) * Cosine(m_DEC), Sine(m_RA) * Cosine(m_DEC), Sine(m_DEC)) + m_LookAtCenter;
}

void Camera::SetPositionWorld(const Point3d& positionWorld)
{
   Point3d currentPosition = PositionWorld();

   if (currentPosition == positionWorld)
      return;

   OffsetPositionWorld(positionWorld - currentPosition);
}

void Camera::OffsetPositionWorld(const Vector3d& offsetWorld)
{
   if (Length(offsetWorld) == 0.0)
      return;

   m_LookAtCenter += offsetWorld;

   c_ViewMatrixDirty = true;
   c_ViewProjectionMatrixDirty = true;
}

//const Vector4d& Camera::GetOrthoBounds() const
//{
//   return m_OrthoBounds;
//}
//
//void Camera::SetOrthoBounds(const Vector4d& orthoBounds)
//{
//   if (m_OrthoBounds == orthoBounds)
//      return;
//
//   m_OrthoBounds = orthoBounds;
//
//   if (!m_PerspectiveProjection)
//   {
//      c_ProjectionMatrixDirty = true;
//      c_ViewProjectionMatrixDirty = true;
//      c_FrustrumPlanes.clear();
//   }
//}

Angled Camera::FieldOfView() const
{
   return m_FieldOfView;
}

void Camera::SetFieldOfView(const Angled fieldOfView)
{
   if (m_FieldOfView == fieldOfView)
      return;

   DCHECK(fieldOfView.Degrees() > 0.0f && fieldOfView.Degrees() < 180.0f);
   m_FieldOfView = fieldOfView;

   if (m_PerspectiveProjection)
   {
      c_ProjectionMatrixDirty = true;
      c_ViewProjectionMatrixDirty = true;
      //c_FrustrumPlanes.clear();
   }
}

double Camera::AspectRatio() const
{
   if (m_AutoAspectRatio)
      return ViewportAspectRatio();
   else
      return m_AspectRatio;
}

void Camera::SetAspectRatio(const double aspect)
{
   if (!m_AutoAspectRatio && m_AspectRatio == aspect)
      return;

   DCHECK(aspect > 0.0);

   if (m_PerspectiveProjection)
   {
      c_ProjectionMatrixDirty = true;
      c_ViewProjectionMatrixDirty = true;
      //c_FrustrumPlanes.clear();
   }
}

void Camera::SetUseAutoAspectRatio()
{
   if (m_AutoAspectRatio)
      return;

   if (m_PerspectiveProjection)
   {
      c_ProjectionMatrixDirty = true;
      c_ViewProjectionMatrixDirty = true;
      //c_FrustrumPlanes.clear();
   }
}

double Camera::NearPlane() const
{
   return m_NearPlane;
}

double Camera::FarPlane() const
{
   return m_FarPlane;
}

void Camera::SetNearAndFarPlanes(const double nearPlane, const double farPlane)
{
   if (nearPlane == m_NearPlane && farPlane == m_FarPlane)
      return;

   DCHECK(farPlane > nearPlane);

   m_NearPlane = nearPlane;
   m_FarPlane = farPlane;

   c_ProjectionMatrixDirty = true;
   c_ViewProjectionMatrixDirty = true;
   //c_FrustrumPlanes.clear();
}

//Rotationd Camera::Orientation() const
//{
//   return m_Orientation;
//}
//
//void Camera::SetOrientation(const Rotationd& orient)
//{
//   if (m_Orientation == orient)
//      return;
//
//   m_Orientation = orient;
//
//   c_ViewMatrixDirty = true;
//   c_ViewProjectionMatrixDirty = true;
//}
//
//const Vector3d& Camera::Scale() const
//{
//   return m_Scale;
//}
//
//void Camera::SetScale(const Vector3d& scale)
//{
//   if (m_Scale == scale)
//      return;
//
//   m_Scale = scale;
//
//   c_ViewMatrixDirty = true;
//   c_ViewProjectionMatrixDirty = true;
//}

void Camera::SetLookAt(const ion::math::Point3d& lookAtCenter) 
{
   if (m_LookAtCenter == lookAtCenter)
      return;

   m_LookAtCenter = lookAtCenter;

   c_ViewMatrixDirty = true;
   c_ViewProjectionMatrixDirty = true;
}

void Camera::SetRA(ion::math::Angled ra)
{
   while (ra >= Angled::FromDegrees(360.0))
      ra -= Angled::FromDegrees(360.0);
   while (ra < Angled::FromDegrees(0.0))
      ra += Angled::FromDegrees(360.0);

   if (m_RA == ra)
      return;

   m_RA = ra;

   c_ViewMatrixDirty = true;
   c_ViewProjectionMatrixDirty = true;
}

void Camera::SetDEC(ion::math::Angled dec) {
   while (dec <= Angled::FromDegrees(-180.0))
      dec += Angled::FromDegrees(360.0);
   while (dec > Angled::FromDegrees(180.0))
      dec -= Angled::FromDegrees(360.0);

   if (m_DEC == dec)
      return;

   m_DEC = dec;

   c_ViewMatrixDirty = true;
   c_ViewProjectionMatrixDirty = true;
}

void Camera::SetRadius(ion::math::Vector1d radius) 
{
   if (radius[0] < 1e-6)
      radius[0] = 1e-6f;

   if (radius[0] > 1e12)
      radius[0] = 1e12f;

   if (m_Radius == radius)
      return;

   m_Radius = radius;

   c_ViewMatrixDirty = true;
   c_ViewProjectionMatrixDirty = true;
}

//void Camera::LookAt(const Point3d& eye, const Point3d& center, const Vector3d& up)
//{
//   DCHECK(eye != center);
//   DCHECK(Length(up) != 0.0);
//
//   c_ViewProjectionMatrixDirty = true;
//
//   //Use the set methods since they will correctly refresh the dirty flags if necessary
//   SetPositionWorld(eye);
//   SetOrientation(Rotationd::FromRotationMatrix(NonhomogeneousSubmatrixH(LookAtMatrixFromCenter(eye, center, up))));//Get the 3x3 top left elements when using the look at method
//}

void Camera::DeltaViewpoint(const ion::math::Angled& deltaRa, const ion::math::Angled& deltaDec, const ion::math::Vector1d& deltaRange) 
{
   SetRA(m_RA + deltaRa);
   SetDEC(m_DEC + deltaDec);
   SetRadius(m_Radius + deltaRange);
}
void Camera::ScaleViewpoint(double scaleRa, double scaleDec, double scaleRange) 
{
   SetRA(m_RA * scaleRa);
   SetDEC(m_DEC * scaleDec);
   SetRadius(m_Radius * scaleRange);
}

void Camera::Perspective(const Angled fov, const double aspect, const double nearPlane, const double farPlane)
{
   m_PerspectiveProjection = true;

   //The following will set the parameters necessary for a perspective calculation
   SetFieldOfView(fov);
   SetAspectRatio(aspect);
   SetNearAndFarPlanes(nearPlane, farPlane);
}

void Camera::Perspective(const Angled fov, const double nearPlane, const double farPlane)
{
   m_PerspectiveProjection = true;

   //The following will set the parameters necessary for a perspective calculation
   SetFieldOfView(fov);
   SetUseAutoAspectRatio();
   SetNearAndFarPlanes(nearPlane, farPlane);
}

//void Camera::Ortho(const Vector4d& bounds)
//{
//   Ortho(bounds, m_NearPlane, m_FarPlane);
//}
//
//void Camera::Ortho(const Vector4d& bounds, const double nearVal, const double farVal)
//{
//   DCHECK(length(bounds) != 0.0);
//
//   m_PerspectiveProjection = false;
//
//   SetOrthoBounds(bounds);
//   SetNearAndFarPlanes(nearVal, farVal);
//}

bool Camera::UsingPerspectiveProj() const
{
   return m_PerspectiveProjection;
}

double Camera::ViewportAspectRatio() const
{
   auto size = m_WindowBounds.GetSize();
   return (double)(size[0]) / (double)(size[1]);
}

void Camera::SetViewportBounds(const Vector2ui& bounds)
{
   SetViewportBounds(Range2ui::BuildWithSize(m_WindowBounds.GetMinPoint(), bounds));
}

void Camera::SetViewportBounds(const Range2ui& bounds)
{
   if (m_WindowBounds == bounds)
      return;

   m_WindowBounds = bounds;

   c_ProjectionMatrixDirty = true;
   c_ViewProjectionMatrixDirty = true;
   //c_FrustrumPlanes.clear();

   //Finally, set the root state table
   m_RootStateTable->SetViewport(Range2i::BuildWithSize(Point2i(m_WindowBounds.GetMinPoint()), Vector2i(m_WindowBounds.GetSize())));
}

const Range2ui& Camera::GetViewportBounds() const
{
   return m_WindowBounds;
}

Point2ui Camera::GetViewportPosition() const
{
   return m_WindowBounds.GetMinPoint();
}

Vector2ui Camera::GetViewportSize() const
{
   return m_WindowBounds.GetSize();
}

uint32_t Camera::GetViewportWidth() const
{
   return m_WindowBounds.GetSize()[0];
}

uint32_t Camera::GetViewportHeight() const
{
   return m_WindowBounds.GetSize()[1];
}

uint32_t Camera::GetViewportX() const
{
   return m_WindowBounds.GetMinPoint()[0];
}

uint32_t Camera::GetViewportY() const
{
   return m_WindowBounds.GetMinPoint()[1];
}

//const Vector3d& Camera::Forward() const
//{
//   return m_Orientation[2];
//}
//
//const Vector3d& Camera::Right() const
//{
//   return m_Orientation[0];
//}
//
//const Vector3d& Camera::Up() const
//{
//   return m_Orientation[1];
//}

ion::math::Vector3d Camera::Up() const {
   return Vector3d(-Cosine(m_RA) * Sine(m_DEC), -Sine(m_RA) * Sine(m_DEC), Cosine(m_DEC));
}

const Matrix4d& Camera::ViewProjectionMatrix() const
{
   if (c_ViewProjectionMatrixDirty)
   {
      c_ViewProjectionMatrix = ProjectionMatrix() * ViewMatrix();
      c_ViewProjectionMatrixDirty = false;
   }

   return c_ViewProjectionMatrix;
}

const Matrix4d& Camera::ProjectionMatrix() const
{
   if (c_ProjectionMatrixDirty)
   {
      if (m_PerspectiveProjection)
      {
         c_ProjectionMatrix = PerspectiveMatrixFromView(m_FieldOfView, AspectRatio(), m_NearPlane, m_FarPlane);
      }
      else
      {
         c_ProjectionMatrix = OrthographicMatrixFromFrustum(m_OrthoBounds[0], m_OrthoBounds[1], m_OrthoBounds[2], m_OrthoBounds[3], m_NearPlane, m_FarPlane);
      }

      c_ProjectionMatrixDirty = false;
   }

   return c_ProjectionMatrix;
}

const Matrix4d& Camera::ViewMatrix() const
{
   if (c_ViewMatrixDirty)
   {
      //Calc the view matrix from scale * orient * translate. This is backwards from the normal order for model matricies
      //c_ViewMatrix = glm::scale(Matrix4d(Orientation()) * glm::translate(Matrix4d(1.0), -m_PositionWorld), m_Scale);

      //c_ViewMatrix = (RotationMatrixH(Orientation()) * TranslationMatrix(-m_PositionWorld)) * ScaleMatrixH(m_Scale);

      c_ViewMatrix = LookAtMatrixFromCenter(PositionWorld(), m_LookAtCenter, Up());
      c_ViewMatrixDirty = false;
   }

   return c_ViewMatrix;
}

//Point3d Camera::Project(const Point3d & worldPosition, const Matrix4d & modelMatrix) const
//{
//   return
//      Project
//      (
//      worldPosition,
//      ViewMatrix() * modelMatrix,
//      ProjectionMatrix(), m_WindowBounds,
//      UsingPerspectiveProj(),
//      IsUsingNegativeDepthBuffer());
//}
//
//Point3d Camera::Project
//(
//   const Point3d & obj,
//   const Matrix4d & model,
//   const Matrix4d & proj,
//   const Vector4ui & viewport,
//   bool isPerspective,
//   bool isUsingNegativeDepthBuffer
//)
//{
//   //Ideally we could just use the glm::project method here but that contains the 0.5*x + 0.5 remap of the depth buffer. With the negative reversed
//   //depth buffer this remap is different so we need to do the calculations manually
//   Vector4d tmp = Vector4d(obj, 1.0);
//   tmp = model * tmp;
//   tmp = proj * tmp;
//
//   tmp /= tmp.w;
//
//   //Now we have the position in clip space. Last step is to convert it to NDC where the depth is in the range [0,1]
//   if (isPerspective && isUsingNegativeDepthBuffer)
//   {
//      //X and Y are mapped normal. Convert Z to the correct range
//      tmp.x = tmp.x * double(0.5) + double(0.5);
//      tmp.y = tmp.y * double(0.5) + double(0.5);
//      tmp.z = 1.0 - tmp.z;
//   }
//   else
//   {
//      tmp = tmp * double(0.5) + double(0.5);
//   }
//
//   tmp[0] = tmp[0] * double(viewport[2]) + double(viewport[0]);
//   tmp[1] = tmp[1] * double(viewport[3]) + double(viewport[1]);
//
//   return Point3d(tmp);
//}

//Vector4d makePlane(const Point3d & p1, const Point3d & p2, const Point3d & p3)
//{
//   Point3d aux1, aux2;
//
//   aux1 = p1 - p2;
//   aux2 = p3 - p2;
//
//   Point3d normal = glm::normalize(glm::cross(aux2, aux1));
//
//   return Vector4d(normal, -glm::dot(normal, p2));
//}
//
//const std::vector<Vector4d>& Camera::FrustrumPlanes() const
//{
//   if (c_FrustrumPlanes.size() == 0)
//   {
//      double nearDist = -NearPlane();
//      double farDist = -FarPlane();
//
//      //These are the half heights and widths to make the calcs below easier
//      double nearHeight = 0.0;
//      double nearWidth = 0.0;
//      double farHeight = 0.0;
//      double farWidth = 0.0;
//
//      //Need to recalc
//      if (m_PerspectiveProjection)
//      {
//         double tang = tan(glm::radians(m_FieldOfView) * 0.5);
//         nearHeight = abs(nearDist) * tang;
//         nearWidth = nearHeight * AspectRatio();
//         farHeight = abs(farDist)  * tang;
//         farWidth = farHeight * AspectRatio();
//      }
//      else
//      {
//         //TODO
//         DEPLOYMENT_WARNING("MDD", "Rob added this to remind demoret.");
//      }
//
//      //Now compute the 8 points of the view frustrum
//      Point3d ntl = Point3d(-nearWidth, nearHeight, nearDist);
//      Point3d ntr = Point3d(nearWidth, nearHeight, nearDist);
//      Point3d nbl = Point3d(-nearWidth, -nearHeight, nearDist);
//      Point3d nbr = Point3d(nearWidth, -nearHeight, nearDist);
//      Point3d ftl = Point3d(-farWidth, farHeight, farDist);
//      Point3d ftr = Point3d(farWidth, farHeight, farDist);
//      Point3d fbl = Point3d(-farWidth, -farHeight, farDist);
//      Point3d fbr = Point3d(farWidth, -farHeight, farDist);
//
//      c_FrustrumPlanes.push_back(makePlane(ntr, ntl, ftl)); //Top
//      c_FrustrumPlanes.push_back(makePlane(nbl, nbr, fbr)); //Bottom
//      c_FrustrumPlanes.push_back(makePlane(ntl, nbl, fbl)); //Left
//      c_FrustrumPlanes.push_back(makePlane(nbr, ntr, fbr)); //Right
//      c_FrustrumPlanes.push_back(makePlane(ntl, ntr, nbr)); //Near
//
//      if (!IsUsingNegativeDepthBuffer()) //No far plane with negative depth buffer
//         c_FrustrumPlanes.push_back(makePlane(ftr, ftl, fbl)); //Far
//   }
//
//   return c_FrustrumPlanes;
//}
//
//bool Camera::IsInFrustrum(const AiCommon::AlignedBoundingBox& box) const
//{
//   const std::vector<Vector4d>& frustrumPlanes = FrustrumPlanes();
//
//   for (int i = 0; i < frustrumPlanes.size(); i++)
//   {
//      Point3d farthestPoint = box.GetMin();
//
//      if (frustrumPlanes[i].x > 0)
//         farthestPoint.x = box.GetMax().x;
//
//      if (frustrumPlanes[i].y > 0)
//         farthestPoint.y = box.GetMax().y;
//
//      if (frustrumPlanes[i].z > 0)
//         farthestPoint.z = box.GetMax().z;
//
//      if (frustrumPlanes[i].w + glm::dot(Point3d(frustrumPlanes[i]), farthestPoint) < 0.0)
//         return false;
//   }
//   return true;
//
//}
