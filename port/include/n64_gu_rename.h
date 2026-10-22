/* Force-included into game code and libultra gu sources: libogc has its own gu* functions (guOrtho, guPerspective,
 * ... with different signatures), so the N64 ones are renamed to n64_gu*. */
#ifndef N64_GU_RENAME_H
#define N64_GU_RENAME_H
#define guFrustum n64_guFrustum
#define guFrustumF n64_guFrustumF
#define guLookAt n64_guLookAt
#define guLookAtF n64_guLookAtF
#define guMtxCatF n64_guMtxCatF
#define guMtxCatL n64_guMtxCatL
#define guMtxF2L n64_guMtxF2L
#define guMtxIdent n64_guMtxIdent
#define guMtxIdentF n64_guMtxIdentF
#define guMtxL2F n64_guMtxL2F
#define guMtxXFMF n64_guMtxXFMF
#define guMtxXFML n64_guMtxXFML
#define guNormalize n64_guNormalize
#define guOrtho n64_guOrtho
#define guOrthoF n64_guOrthoF
#define guPerspective n64_guPerspective
#define guPerspectiveF n64_guPerspectiveF
#define guPosition n64_guPosition
#define guPositionF n64_guPositionF
#define guRotate n64_guRotate
#define guRotateF n64_guRotateF
#define guRotateRPY n64_guRotateRPY
#define guRotateRPYF n64_guRotateRPYF
#define guScale n64_guScale
#define guScaleF n64_guScaleF
#define guTranslate n64_guTranslate
#define guTranslateF n64_guTranslateF
#endif
